// 阿里云盘分享解析核心流程(走网页端 API,官方 OpenAPI 不支持消费他人分享)
// refresh_token -> access_token -> get_share_token(匿名列目录)
// -> get_share_link_download_url(需 access_token) 直接取分享直链,无需转存
// 协议参考公开的网页端分享接口约定,实现为原创。

const API_HOST = "https://api.alipan.com";
const AUTH_HOST = "https://auth.alipan.com";
const REFERER = "https://www.alipan.com/";
const X_CANARY = "client=web,app=share,version=v2.3.1";
// 设备签名(过下载风控)用的 Android canary 与 appId
const CANARY_SIGN = "client=Android,app=adrive,version=v4.1.0";
const SIGN_APP_ID = "5dde4e1bdf9e4966b387ba58f4b3fdc3";
// OpenList 全局 UA(含 OpenList/x 标记):部分网关只对该端点做 UA 过滤,直连分享直链需精确匹配
const OPENLIST_UA =
    "Mozilla/5.0 (Macintosh; Apple macOS 26_1_0) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Safari/537.36 Chrome/142.0.0.0 OpenList/425.6.30";
const UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Chrome/114.0.0.0 Safari/537.36";

// 弹窗请求提取码;返回用户输入,取消或通道不可用返回 null
async function promptExtractionCode(message) {
    try {
        const code = await gdl.ui.requestVerification({ message: message });
        return code ? String(code).trim() : null;
    } catch (e) {
        gdl.log.info("verification prompt unavailable or cancelled: " + e);
        return null;
    }
}

export class AliApi {
    constructor() {
        this.reset();
    }

    reset() {
        this.refreshToken = "";
        this.accessToken = "";
        this.shareId = "";
        this.sharePwd = "";
        this.shareToken = "";
        // file_id -> drive_id(取直链需要)
        this.driveByFid = {};
        // 设备签名会话(下载风控)
        this.userId = "";
        this.deviceId = "";
        this.signature = "";
        this.myDriveId = "";
        this.deviceReady = false;
    }

    // 随机 UUID v4(x-request-id 用)
    uuid() {
        const hex = "0123456789abcdef";
        let u = "";
        for (let i = 0; i < 36; i++) {
            if (i === 8 || i === 13 || i === 18 || i === 23) u += "-";
            else if (i === 14) u += "4";
            else if (i === 19) u += hex[(Math.floor(Math.random() * 4) + 8)];
            else u += hex[Math.floor(Math.random() * 16)];
        }
        return u;
    }

    // 建立设备签名会话:取 user_id -> 确定性派生设备密钥/公钥/签名 -> create_session 注册
    // deviceId 与私钥同源 = SHA256(userId);nonce 恒为 0,故签名值恒定
    async ensureDeviceSession() {
        if (this.deviceReady) return true;
        if (!(await this.ensureAccessToken())) return false;

        // 取 user_id(Authorization 用 Bearer + TAB,阿里的隐蔽约定)
        const uResp = await this.postRetry(API_HOST + "/v2/user/get", {
            json: {},
            headers: this.baseHeaders({
                "Authorization": "Bearer\t" + this.accessToken,
                "X-Canary": CANARY_SIGN,
            }),
            timeout: 15000,
        }, "user_get");
        if (!uResp || uResp.status !== 200) {
            gdl.log.warn("ali user/get failed");
            return false;
        }
        let udoc;
        try {
            udoc = uResp.json();
        } catch (e) {
            return false;
        }
        if (typeof udoc.user_id !== "string" || !udoc.user_id) {
            gdl.log.warn("ali user_id missing");
            return false;
        }
        this.userId = udoc.user_id;
        this.myDriveId = typeof udoc.default_drive_id === "string" ? udoc.default_drive_id : "";

        // 确定性派生:seed = SHA256(userId) 既是 deviceId 也是私钥标量
        const seed = gdl.crypto.sha256(this.userId);
        this.deviceId = seed;
        const pubKey = gdl.crypto.secp256k1PubKey(seed);
        const signMsg = SIGN_APP_ID + ":" + seed + ":" + this.userId + ":0";
        this.signature = gdl.crypto.secp256k1Sign(seed, gdl.crypto.sha256(signMsg));

        const cResp = await this.postRetry(API_HOST + "/users/v1/users/device/create_session", {
            json: {
                deviceName: "samsung",
                modelName: "SM-G9810",
                nonce: 0,
                pubKey: pubKey,
                refreshToken: this.refreshToken,
            },
            headers: this.baseHeaders({
                "Authorization": "Bearer\t" + this.accessToken,
                "X-Signature": this.signature,
                "X-Device-Id": this.deviceId,
                "X-Canary": CANARY_SIGN,
                "x-request-id": this.uuid(),
            }),
            timeout: 15000,
        }, "create_session");
        if (!cResp || cResp.status !== 200) {
            gdl.log.warn("ali create_session http " + (cResp ? cResp.status : "null"));
            return false;
        }
        let cdoc;
        try {
            cdoc = cResp.json();
        } catch (e) {
            cdoc = {};
        }
        if (cdoc.code) {
            gdl.log.warn("ali create_session error: " + cdoc.code);
            return false;
        }
        this.deviceReady = true;
        return true;
    }

    // 设备签名请求头(个人端点需要):Bearer+TAB + X-Device-Id + X-Signature + Android canary
    signedHeaders(extra) {
        return this.baseHeaders(Object.assign({
            "Authorization": "Bearer\t" + this.accessToken,
            "X-Device-Id": this.deviceId,
            "X-Signature": this.signature,
            "X-Canary": CANARY_SIGN,
            "x-request-id": this.uuid(),
        }, extra || {}));
    }

    // 转存分享文件到自己网盘(batch copy),返回自己网盘里的新 file_id
    async transferShareFile(info) {
        const resp = await this.postRetry(API_HOST + "/adrive/v2/batch", {
            json: {
                requests: [{
                    body: {
                        file_id: info.file_id,
                        share_id: this.shareId,
                        auto_rename: true,
                        to_parent_file_id: "root",
                        to_drive_id: this.myDriveId,
                    },
                    headers: { "Content-Type": "application/json" },
                    id: "0",
                    method: "POST",
                    url: "/file/copy",
                }],
                resource: "file",
            },
            headers: this.signedHeaders({ "x-share-token": this.shareToken }),
            timeout: 15000,
        }, "transfer");
        if (!resp || resp.status !== 200) {
            gdl.log.warn("ali transfer http " + (resp ? resp.status : "null"));
            return null;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return null;
        }
        const responses = Array.isArray(doc.responses) ? doc.responses : [];
        if (responses.length && responses[0].body && typeof responses[0].body.file_id === "string") {
            return responses[0].body.file_id;
        }
        gdl.log.warn("ali transfer no file_id (folder or failed)");
        return null;
    }

    // 个人网盘取下载直链(设备签名端点)
    async getPersonalDownloadUrl(fileId) {
        const resp = await this.postRetry(API_HOST + "/v2/file/get_download_url", {
            json: { drive_id: this.myDriveId, file_id: fileId },
            headers: this.signedHeaders(),
            timeout: 15000,
        }, "get_download_url");
        if (!resp || resp.status !== 200) {
            gdl.log.warn("ali get_download_url http " + (resp ? resp.status : "null"));
            return null;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return null;
        }
        return typeof doc.url === "string" && doc.url ? doc.url : null;
    }

    // 清理转存文件(移入回收站,best effort)
    async deleteOwnFile(fileId) {
        try {
            await this.postRetry(API_HOST + "/v2/recyclebin/trash", {
                json: { drive_id: this.myDriveId, file_id: fileId },
                headers: this.signedHeaders(),
                timeout: 15000,
            }, "trash");
        } catch (e) { /* 忽略 */ }
    }

    baseHeaders(extra) {
        const h = {
            "User-Agent": UA,
            "Content-Type": "application/json",
            "Referer": REFERER,
            "Origin": "https://www.alipan.com",
        };
        return extra ? Object.assign(h, extra) : h;
    }

    // 带瞬时错误重试的 POST(阿里 api 偶发 TLS 握手/连接重置)
    async postRetry(url, opts, label) {
        let lastErr = "";
        for (let i = 0; i < 5; i++) {
            try {
                return await gdl.http.post(url, opts);
            } catch (e) {
                lastErr = String(e);
                await gdl.utils.sleep(500 + i * 500); // 递增退避:0.5s,1s,1.5s,2s
            }
        }
        gdl.log.warn("ali " + label + " request failed after retries: " + lastErr);
        return null;
    }

    // refresh_token -> access_token(web 端)
    async ensureAccessToken() {
        if (this.accessToken) return true;
        if (!this.refreshToken) return false;
        const resp = await this.postRetry(AUTH_HOST + "/v2/account/token", {
            json: { refresh_token: this.refreshToken, grant_type: "refresh_token" },
            headers: this.baseHeaders(),
            timeout: 15000,
        }, "token");
        if (!resp) return false;
        if (resp.status !== 200) {
            gdl.log.warn("ali token http " + resp.status);
            return false;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return false;
        }
        if (typeof doc.access_token !== "string" || !doc.access_token) {
            gdl.log.warn("ali token error: " + (doc.message || doc.code || "no access_token"));
            if (doc.message) gdl.notify(doc.message, "error");
            return false;
        }
        this.accessToken = doc.access_token;
        return true;
    }

    // 分享口令 -> share_token(匿名可调);返回 {ok:true} 或 {ok:false, code, message}
    async fetchShareToken() {
        const resp = await this.postRetry(API_HOST + "/v2/share_link/get_share_token", {
            json: { share_id: this.shareId, share_pwd: this.sharePwd || "" },
            headers: this.baseHeaders(),
            timeout: 15000,
        }, "share_token");
        if (!resp) return { ok: false, message: "network unreachable" };
        if (resp.status !== 200 && resp.status !== 400 && resp.status !== 403) {
            gdl.log.warn("ali share_token http " + resp.status);
            return { ok: false, message: "http " + resp.status };
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return { ok: false, message: "bad response" };
        }
        if (typeof doc.share_token !== "string" || !doc.share_token) {
            gdl.log.warn("ali share_token error: " + (doc.message || doc.code));
            return { ok: false, code: String(doc.code || ""), message: String(doc.message || "") };
        }
        this.shareToken = doc.share_token;
        return { ok: true };
    }

    // 列分享目录(分页 marker)
    async listDir(parentFileId, basePath) {
        const out = [];
        let marker = "";
        for (let guard = 0; guard < 100; guard++) {
            const body = {
                share_id: this.shareId,
                parent_file_id: parentFileId,
                limit: 200,
                order_by: "name",
                order_direction: "ASC",
            };
            if (marker) body.marker = marker;
            const resp = await this.postRetry(API_HOST + "/adrive/v3/file/list", {
                json: body,
                headers: this.baseHeaders({ "x-share-token": this.shareToken, "X-Canary": X_CANARY }),
                timeout: 15000,
            }, "list");
            if (!resp) return out.length ? out : null;
            if (resp.status !== 200) {
                gdl.log.warn("ali list http " + resp.status);
                return out.length ? out : null;
            }
            let doc;
            try {
                doc = resp.json();
            } catch (e) {
                return out.length ? out : null;
            }
            if (doc.code) {
                // share_token 过期则重取一次
                if (String(doc.code).indexOf("ShareLinkTokenInvalid") >= 0 && (await this.fetchShareToken()).ok) {
                    guard--;
                    continue;
                }
                gdl.log.warn("ali list error: " + (doc.message || doc.code));
                return out.length ? out : null;
            }
            const items = Array.isArray(doc.items) ? doc.items : [];
            for (const item of items) {
                if (typeof item.file_id !== "string" || typeof item.name !== "string") continue;
                const isDir = item.type === "folder";
                this.driveByFid[item.file_id] = typeof item.drive_id === "string" ? item.drive_id : "";
                out.push({
                    name: item.name,
                    path: basePath === "/" ? "/" + item.name : basePath + "/" + item.name,
                    size: typeof item.size === "number" ? item.size : 0,
                    is_dir: isDir,
                    file_id: item.file_id,
                    create_time: 0,
                    root_path: "/",
                });
            }
            marker = typeof doc.next_marker === "string" ? doc.next_marker : "";
            if (!marker || items.length === 0) break;
        }
        return out;
    }

    // 组装下载结果(阿里直链需带 Referer,否则 403)
    makeResult(info, url) {
        return {
            real_url: url,
            file_name: info.name,
            file_size: info.size || 0,
            headers: {
                "header": "Referer:" + REFERER,
                "user-agent": UA,
            },
            options: {},
            mirrors: [],
        };
    }

    // 直连分享直链端点(residential 网络通常可用,不动网盘);被网关拦截(410)时返回 null
    async fetchShareDownloadDirect(info) {
        const driveId = this.driveByFid[info.file_id] || "";
        const resp = await this.postRetry(API_HOST + "/v2/file/get_share_link_download_url", {
            json: {
                drive_id: driveId,
                file_id: info.file_id,
                expire_sec: 600,
                share_id: this.shareId,
            },
            // 精确复刻 OpenList:仅 4 个头 + OpenList UA(含 OpenList 标记),Bearer+TAB
            headers: {
                "User-Agent": OPENLIST_UA,
                "Content-Type": "application/json",
                "Authorization": "Bearer\t" + this.accessToken,
                "X-Canary": X_CANARY,
                "x-share-token": this.shareToken,
            },
            timeout: 15000,
        }, "share_download");
        if (!resp || resp.status !== 200) {
            gdl.log.warn("ali direct share download http " + (resp ? resp.status : "null"));
            return null;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return null;
        }
        const url = typeof doc.download_url === "string" ? doc.download_url
            : (typeof doc.url === "string" ? doc.url : "");
        return url ? this.makeResult(info, url) : null;
    }

    // 转存路径(直连被拦截时的回退):转存到自己网盘 -> 个人下载端点(设备签名)取直链 -> 清理
    async fetchViaTransfer(info) {
        if (!(await this.ensureDeviceSession())) {
            gdl.notify("Aliyundrive device session failed (check refresh_token)", "error");
            return null;
        }
        const ownFid = await this.transferShareFile(info);
        if (!ownFid) {
            gdl.notify("Aliyundrive transfer failed", "error");
            return null;
        }
        const url = await this.getPersonalDownloadUrl(ownFid);
        await this.deleteOwnFile(ownFid);
        return url ? this.makeResult(info, url) : null;
    }

    // 取分享文件真实下载直链:优先直连端点,被网关拦截时回退转存
    async fetchDownloadUrl(info) {
        if (!(await this.ensureAccessToken())) {
            gdl.notify("Aliyundrive download requires a refresh_token (set it in plugin settings)", "error");
            return null;
        }
        const direct = await this.fetchShareDownloadDirect(info);
        if (direct) return direct;
        gdl.log.info("ali direct share download unavailable, falling back to transfer");
        return this.fetchViaTransfer(info);
    }

    // ---- 对外主流程 ----

    async parseUrl(url, userToken) {
        gdl.log.info("ali parse started");
        this.reset();
        this.refreshToken = (userToken || "").trim();

        const m = url.match(/\/s\/([0-9a-zA-Z]+)/);
        if (!m) {
            gdl.log.warn("ali invalid share url");
            return null;
        }
        this.shareId = m[1];
        const pm = url.match(/[?&#](?:pwd|share_pwd|passcode)=([^&\s]+)/);
        this.sharePwd = pm ? gdl.utils.urlDecode(pm[1]) : "";

        let r = await this.fetchShareToken();
        for (let attempt = 0; !r.ok && attempt < 3; attempt++) {
            // 缺码或错误码/信息指向 share_pwd 时进入弹窗补码
            const errText = (r.code || "") + " " + (r.message || "");
            const pwdRelated = !this.sharePwd || /sharepwd|share_pwd|password|提取码|密码/i.test(errText);
            if (!pwdRelated) break;
            const tip = (!this.sharePwd && attempt === 0)
                ? "This share link requires an extraction code."
                : "Wrong extraction code, please try again.";
            const code = await promptExtractionCode(tip);
            if (code === null) {
                gdl.notify("Parsing cancelled: the share link requires an extraction code.", "warning");
                return null;
            }
            this.sharePwd = code;
            r = await this.fetchShareToken();
        }
        if (!r.ok) {
            gdl.log.warn("ali share token failed");
            if (r.message) gdl.notify(r.message, "error");
            return null;
        }
        const files = await this.listDir("root", "/");
        if (!files) {
            gdl.log.warn("ali share list failed");
            return null;
        }
        gdl.log.info("ali parse completed files=" + files.length);
        return files;
    }

    async enterDirectory(info) {
        if (!this.shareToken) {
            gdl.log.warn("ali enterDirectory without session");
            return null;
        }
        return this.listDir(info.file_id, info.path);
    }

    async getDownloadInfo(info) {
        if (info.is_dir) {
            const files = await this.enterDirectory(info);
            if (!files) return null;
            const results = [];
            for (const file of files) {
                const sub = await this.getDownloadInfo(file);
                if (sub) results.push(...sub);
            }
            return results;
        }
        const result = await this.fetchDownloadUrl(info);
        return result ? [result] : null;
    }
}
