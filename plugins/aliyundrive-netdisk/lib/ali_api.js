// 阿里云盘分享解析核心流程(走网页端 API,官方 OpenAPI 不支持消费他人分享)
// refresh_token -> access_token -> get_share_token(匿名列目录)
// -> get_share_link_download_url(需 access_token) 直接取分享直链,无需转存
// 协议参考公开的网页端分享接口约定,实现为原创。

const API_HOST = "https://api.alipan.com";
const AUTH_HOST = "https://auth.alipan.com";
const REFERER = "https://www.alipan.com/";
const X_CANARY = "client=web,app=share,version=v2.3.1";
const UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Chrome/114.0.0.0 Safari/537.36";

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

    // refresh_token -> access_token(web 端)
    async ensureAccessToken() {
        if (this.accessToken) return true;
        if (!this.refreshToken) return false;
        let resp;
        try {
            resp = await gdl.http.post(AUTH_HOST + "/v2/account/token", {
                json: { refresh_token: this.refreshToken, grant_type: "refresh_token" },
                headers: this.baseHeaders(),
                timeout: 15000,
            });
        } catch (e) {
            gdl.log.warn("ali token refresh failed: " + e);
            return false;
        }
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

    // 分享口令 -> share_token(匿名可调)
    async fetchShareToken() {
        let resp;
        try {
            resp = await gdl.http.post(API_HOST + "/v2/share_link/get_share_token", {
                json: { share_id: this.shareId, share_pwd: this.sharePwd || "" },
                headers: this.baseHeaders(),
                timeout: 15000,
            });
        } catch (e) {
            gdl.log.warn("ali share_token failed: " + e);
            return false;
        }
        if (resp.status !== 200) {
            gdl.log.warn("ali share_token http " + resp.status);
            return false;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return false;
        }
        if (typeof doc.share_token !== "string" || !doc.share_token) {
            gdl.log.warn("ali share_token error: " + (doc.message || doc.code));
            if (doc.message) gdl.notify(doc.message, "error");
            return false;
        }
        this.shareToken = doc.share_token;
        return true;
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
            let resp;
            try {
                resp = await gdl.http.post(API_HOST + "/adrive/v3/file/list", {
                    json: body,
                    headers: this.baseHeaders({ "x-share-token": this.shareToken, "X-Canary": X_CANARY }),
                    timeout: 15000,
                });
            } catch (e) {
                gdl.log.warn("ali list failed: " + e);
                return out.length ? out : null;
            }
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
                if (String(doc.code).indexOf("ShareLinkTokenInvalid") >= 0 && await this.fetchShareToken()) {
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

    // 取分享文件真实下载直链
    async fetchDownloadUrl(info) {
        if (!(await this.ensureAccessToken())) {
            gdl.notify("Aliyundrive download requires a refresh_token (set it in plugin settings)", "error");
            return null;
        }
        const driveId = this.driveByFid[info.file_id] || "";
        let resp;
        try {
            resp = await gdl.http.post(API_HOST + "/v2/file/get_share_link_download_url", {
                json: {
                    share_id: this.shareId,
                    file_id: info.file_id,
                    drive_id: driveId,
                    expire_sec: 600,
                },
                headers: this.baseHeaders({
                    "x-share-token": this.shareToken,
                    "X-Canary": X_CANARY,
                    "Authorization": "Bearer " + this.accessToken,
                }),
                timeout: 15000,
            });
        } catch (e) {
            gdl.log.warn("ali download url failed: " + e);
            return null;
        }
        if (resp.status !== 200) {
            gdl.log.warn("ali download url http " + resp.status);
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
        if (!url) {
            gdl.log.warn("ali download url error: " + (doc.message || doc.code));
            if (doc.message) gdl.notify(doc.message, "error");
            return null;
        }
        // 阿里直链需带 Referer,否则 403
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

        if (!(await this.fetchShareToken())) {
            gdl.log.warn("ali share token failed");
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
