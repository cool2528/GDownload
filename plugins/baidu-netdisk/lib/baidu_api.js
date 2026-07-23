// 百度网盘分享解析核心流程（对应 baidu_pcs_api.cxx::BaiduPcsApi）
import { DpLogId, generateRandomFloat, nowSeconds } from "./util.js";
import { getRealDownloadAddress, CLIENT_UA } from "./pcs.js";

const UA_WEB =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36";
const PAN_HOST = "https://pan.baidu.com";
const APP_ID = "250528";

// 从 Set-Cookie 响应头（数组或字符串）中提取指定 cookie 值
function extractSetCookieValue(headers, name) {
    let raw = headers["set-cookie"];
    if (!raw) return "";
    if (!Array.isArray(raw)) raw = [raw];
    for (const line of raw) {
        const m = line.match(new RegExp("(?:^|[;\\s])" + name + "=([^;]+)"));
        if (m) return m[1];
    }
    return "";
}

// 从用户 cookie 串中提取指定 cookie
function extractCookieFromString(cookieStr, name) {
    const m = cookieStr.match(new RegExp("(?:^|[;\\s])" + name + "=([^;]+)"));
    return m ? m[1] : "";
}

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

export class BaiduApi {
    constructor() {
        this.logId = "";
        this.surl = "";
        this.uk = "";
        this.shareId = "";
        this.jsToken = "";
        this.bdsToken = "";
        this.isVip = "";
        this.rootPath = "";
        this.randsk = "";
        this.bduss = "";
        this.pcsState = {}; // { uid }
    }

    // 对应 ProcessBaiduCookies：仅保留关键登录 cookie，注入 Jar 并缓存 BDUSS
    processCookies(userToken) {
        if (!userToken) return;
        const keep = ["BDUSS_BFESS", "STOKEN", "PANPSC", "BDUSS"];
        const parts = [];
        for (const name of keep) {
            const v = extractCookieFromString(userToken, name);
            if (v) parts.push(name + "=" + v);
        }
        this.bduss = extractCookieFromString(userToken, "BDUSS") ||
            extractCookieFromString(userToken, "BDUSS_BFESS");
        if (parts.length > 0) {
            // 注入到 pan.baidu.com（host-only），后续列表请求自动携带
            gdl.http.cookies.setFromString("pan.baidu.com", parts.join("; "));
        }
    }

    validateShareUrl(url) {
        return /^https:\/\/pan\.baidu\.com\//.test(url);
    }

    // 对应 FetchInitialPage：不跟随重定向，取 BAIDUID → base64 → logid
    async fetchInitialPage(url) {
        const resp = await gdl.http.get(url, {
            follow_redirects: false,
            timeout: 12000,
        });
        // BAIDUID 可能本次响应新下发，也可能已存在于持久化 Cookie Jar 中
        // （Jar 已带上时服务器不再重发）。优先响应，兜底查 Jar，保证跨会话稳定。
        let baiduId = extractSetCookieValue(resp.headers, "BAIDUID");
        if (!baiduId) {
            for (const c of gdl.http.cookies.list("baidu.com")) {
                if (c.name === "BAIDUID") {
                    baiduId = c.value;
                    break;
                }
            }
        }
        if (!baiduId) {
            return null;
        }
        this.logId = gdl.utils.base64Encode(baiduId);
        return resp;
    }

    // 对应 GetRedirectUrl：拼接 Location 绝对地址
    getRedirectUrl(headers) {
        let loc = headers["location"];
        if (Array.isArray(loc)) loc = loc[0];
        if (!loc) return "";
        return PAN_HOST + loc;
    }

    // 对应 ExtractSurl
    extractSurl(url, hasPassword) {
        try {
            if (hasPassword) {
                const m = url.match(/[?&]surl=([\w-]+)/);
                if (m) {
                    this.surl = m[1];
                    return this.surl;
                }
            }
            let m = url.match(/pan\.baidu\.com\/s\/1([\w-]+)/);
            if (!m) m = url.match(/pan\.baidu\.com\/share\/init\?surl=([\w-]+)/);
            if (!m) m = url.match(/surl=([\w-]+)/);
            this.surl = m ? m[1] : "";
            return this.surl;
        } catch (e) {
            return "";
        }
    }

    // 对应 VerifySharePassword：POST /share/verify 取 randsk
    async verifySharePassword(surl, pwd, refererUrl) {
        const timestamp = String(Date.now());
        const dpLogId = new DpLogId().getDpLogId();
        const resp = await gdl.http.post("https://pan.baidu.com/share/verify", {
            params: {
                t: timestamp, surl: surl, web: "1", app_id: APP_ID,
                bdstoken: this.bdsToken, logid: this.logId, clienttype: "0", "dp-logid": dpLogId,
            },
            form: { pwd: pwd, vcode: "", vcode_str: "" },
            headers: { "User-Agent": UA_WEB, "Referer": refererUrl },
        });
        if (resp.status !== 200) {
            gdl.log.warn("verify password status " + resp.status);
            return false;
        }
        try {
            const doc = resp.json();
            if (doc.error && doc.error !== 0) return false;
            if (typeof doc.randsk !== "string" || !doc.randsk) return false;
            this.randsk = doc.randsk;
            return true;
        } catch (e) {
            gdl.log.warn("verify password parse failed: " + e);
            return false;
        }
    }

    // 对应 ExtractShareInfo：从分享页 html 提取 bdstoken/shareid/uk/is_vip/root_path
    extractShareInfo(html) {
        try {
            let hasContext = false;
            const m = html.match(/locals\.mset\((\{.*?\})\);/);
            if (m) {
                const data = JSON.parse(m[1]);
                hasContext = true;
                if (typeof data.bdstoken === "string") this.bdsToken = data.bdstoken;
                if (typeof data.shareid === "number") this.shareId = String(data.shareid);
                if (typeof data.share_uk === "string") this.uk = data.share_uk;
                else if (typeof data.share_uk === "number") this.uk = String(data.share_uk);
                if (typeof data.is_vip === "number") this.isVip = String(data.is_vip);
                if (Array.isArray(data.file_list)) {
                    for (const item of data.file_list) {
                        if (item && typeof item.parent_path === "string") {
                            this.rootPath = item.parent_path;
                            break;
                        }
                    }
                }
            }
            if (!hasContext) {
                hasContext = /window\.yunData\s*=\s*\{.*?\};/.test(html);
            }
            // 字段级兜底
            if (!this.shareId) {
                const sm = html.match(/["']?shareid["']?\s*[:=]\s*["']?([0-9]+)/);
                if (sm) this.shareId = sm[1];
            }
            if (!this.uk) {
                const um = html.match(/["']?share_uk["']?\s*[:=]\s*["']?([0-9]+)/);
                if (um) this.uk = um[1];
            }
            this.extractJsToken(html);
            return hasContext && !!this.shareId && !!this.uk;
        } catch (e) {
            gdl.log.warn("extractShareInfo failed: " + e);
            return false;
        }
    }

    extractJsToken(html) {
        const patterns = [/fn%28%22([A-Za-z0-9_]+)%22\)/, /fn\(["']([A-Za-z0-9_]+)["']\)/];
        for (const p of patterns) {
            const m = html.match(p);
            if (m) {
                this.jsToken = m[1];
                return true;
            }
        }
        this.jsToken = "";
        return false;
    }

    // 对应 ReportUserBehavior：埋点，失败不影响解析
    async reportUserBehavior(refererUrl) {
        try {
            const resp = await gdl.http.post(PAN_HOST + "/api/report/user", {
                params: {
                    channel: "chunlei", web: "1", app_id: APP_ID, bdstoken: this.bdsToken,
                    logid: gdl.utils.urlDecode(this.logId), clienttype: "0",
                    "dp-logid": new DpLogId().getDpLogId(),
                },
                form: { timestamp: String(nowSeconds()), action: "web_unlogin_share_browse" },
                headers: { "User-Agent": UA_WEB, "Referer": refererUrl },
            });
            return resp.status === 200;
        } catch (e) {
            return false;
        }
    }

    // 对应 ParseFileList
    parseFileList(jsonText) {
        try {
            const json = typeof jsonText === "string" ? JSON.parse(jsonText) : jsonText;
            const result = [];
            if (Array.isArray(json.list)) {
                for (const item of json.list) {
                    if (typeof item.server_filename !== "string" || typeof item.path !== "string") {
                        continue;
                    }
                    const file = {
                        name: item.server_filename,
                        path: item.path,
                        size: 0,
                        is_dir: false,
                        file_id: "",
                        create_time: 0,
                        root_path: this.rootPath,
                    };
                    if (typeof item.server_mtime === "string") file.create_time = parseInt(item.server_mtime, 10) || 0;
                    else if (typeof item.server_mtime === "number") file.create_time = item.server_mtime;
                    if (typeof item.fs_id === "string") file.file_id = item.fs_id;
                    else if (typeof item.fs_id === "number") file.file_id = String(item.fs_id);
                    if (typeof item.isdir === "string") file.is_dir = item.isdir === "1";
                    else if (typeof item.isdir === "number") file.is_dir = item.isdir === 1;
                    if (typeof item.size === "string") file.size = parseInt(item.size, 10) || 0;
                    else if (typeof item.size === "number") file.size = item.size;
                    result.push(file);
                }
                if (!this.uk || !this.shareId) {
                    if (typeof json.share_id === "number") this.shareId = String(json.share_id);
                    if (typeof json.uk === "number") this.uk = String(json.uk);
                }
            }
            return result;
        } catch (e) {
            gdl.log.warn("parseFileList failed: " + e);
            return null;
        }
    }

    // 对应 FetchShareFileList
    async fetchShareFileList(surl, refererUrl) {
        const realUrl = "https://pan.baidu.com/s/1" + surl;
        let resp = await gdl.http.get(realUrl, {
            follow_redirects: false,
            headers: { "User-Agent": UA_WEB, "Referer": refererUrl },
        });
        if (resp.status !== 200) return null;
        if (!this.extractShareInfo(resp.text())) {
            gdl.log.warn("share page metadata format unsupported");
            return null;
        }
        await this.reportUserBehavior(realUrl);

        resp = await gdl.http.get(PAN_HOST + "/share/list", {
            params: {
                web: "1", app_id: APP_ID, desc: "1", showempty: "0", page: "1", num: "100",
                order: "time", shorturl: surl, root: "1", view_mode: "1", channel: "chunlei",
                bdstoken: this.bdsToken, logid: this.logId, clienttype: "0",
                "dp-logid": new DpLogId().getDpLogId(),
            },
            headers: { "User-Agent": UA_WEB, "Referer": realUrl },
        });
        if (resp.status !== 200) return null;
        return this.parseFileList(resp.text());
    }

    // ---- 对外主流程 ----

    async parseUrl(url, userToken) {
        gdl.log.info("baidu parse started");
        this.processCookies(userToken);
        if (!this.validateShareUrl(url)) {
            gdl.log.warn("invalid baidu share url");
            return null;
        }
        const pwdMatch = url.match(/[?&]pwd=([^&]+)/);
        let pwd = pwdMatch ? pwdMatch[1] : "";
        const hasPassword = !!pwdMatch;

        const initial = await this.fetchInitialPage(url);
        if (!initial) {
            gdl.log.warn("baidu initial page failed");
            return null;
        }
        let newUrl = url;
        let needVerify = hasPassword || !!pwd;
        const redirectUrl = this.getRedirectUrl(initial.headers);
        if (hasPassword) {
            if (!redirectUrl) {
                gdl.log.warn("password redirect location missing");
                return null;
            }
            newUrl = redirectUrl;
            const redirect = await gdl.http.get(newUrl, { headers: { "User-Agent": UA_WEB } });
            if (redirect.status !== 200 && redirect.status !== 302) {
                gdl.log.warn("password redirect page failed");
                return null;
            }
        } else if (redirectUrl && redirectUrl.indexOf("share/init") >= 0) {
            // 链接未携带提取码但分享受保护:改走弹窗补码流程
            gdl.log.info("baidu share requires extraction code, prompting user");
            needVerify = true;
            newUrl = redirectUrl;
            const redirect = await gdl.http.get(newUrl, { headers: { "User-Agent": UA_WEB } });
            if (redirect.status !== 200 && redirect.status !== 302) {
                gdl.log.warn("share init page failed");
                return null;
            }
        }

        const surl = this.extractSurl(newUrl, hasPassword);
        if (!surl) {
            gdl.log.warn("share id not found");
            return null;
        }

        if (needVerify) {
            let verified = pwd ? await this.verifySharePassword(surl, pwd, newUrl) : false;
            for (let attempt = 0; !verified && attempt < 3; attempt++) {
                const tip = (attempt === 0 && !pwd)
                    ? "This share link requires an extraction code."
                    : "Wrong extraction code, please try again.";
                const code = await promptExtractionCode(tip);
                if (code === null) {
                    gdl.notify("Parsing cancelled: the share link requires an extraction code.", "warning");
                    return null;
                }
                pwd = code;
                verified = await this.verifySharePassword(surl, pwd, newUrl);
            }
            if (!verified) {
                gdl.log.warn("password verification failed");
                gdl.notify("Extraction code verification failed.", "error");
                return null;
            }
        }

        const list = await this.fetchShareFileList(surl, newUrl);
        if (!list) {
            gdl.log.warn("file list request failed");
            return null;
        }
        for (const file of list) {
            file.path = gdl.utils.urlDecode(this.rootPath) + "/" + file.name;
        }
        gdl.log.info("baidu parse completed files=" + list.length);
        return list;
    }

    async getShareHomeDirectoryFileList() {
        const realUrl = "https://pan.baidu.com/s/1" + this.surl;
        const resp = await gdl.http.get(PAN_HOST + "/share/list", {
            params: {
                web: "1", app_id: APP_ID, desc: "1", showempty: "0", page: "1", num: "100",
                order: "time", shorturl: this.surl, root: "1", view_mode: "1", channel: "chunlei",
                bdstoken: this.bdsToken, logid: this.logId, clienttype: "0",
                "dp-logid": new DpLogId().getDpLogId(),
            },
            headers: { "User-Agent": UA_WEB, "Referer": realUrl },
        });
        if (resp.status !== 200) return null;
        const list = this.parseFileList(resp.text());
        if (!list) return null;
        for (const file of list) {
            file.path = gdl.utils.urlDecode(this.rootPath) + "/" + file.name;
        }
        return list;
    }

    async enterDirectory(info) {
        if (info.path === gdl.utils.urlDecode(this.rootPath)) {
            return this.getShareHomeDirectoryFileList();
        }
        const surlPath = this.surl.startsWith("1") ? this.surl : "1" + this.surl;
        const realUrl = PAN_HOST + "/s/" + surlPath;
        const resp = await gdl.http.get(PAN_HOST + "/share/list", {
            params: {
                is_from_web: "true", sekey: gdl.utils.urlDecode(this.randsk), uk: this.uk,
                shareid: this.shareId, order: "other", desc: "1", showempty: "0", view_mode: "1",
                web: "1", page: "1", num: "100", dir: info.path, t: generateRandomFloat(),
                channel: "chunlei", app_id: APP_ID, bdstoken: this.bdsToken, logid: this.logId,
                clienttype: "0", "dp-logid": new DpLogId().getDpLogId(),
            },
            headers: { "User-Agent": UA_WEB, "Referer": realUrl },
        });
        if (resp.status !== 200) return null;
        return this.parseFileList(resp.text());
    }

    // 对应 TransferShareFile：转存分享文件到用户网盘
    async transferShareFile(info) {
        const resp = await gdl.http.post(PAN_HOST + "/share/transfer", {
            params: {
                shareid: this.shareId, from: this.uk, sekey: gdl.utils.urlDecode(this.randsk),
                ondup: "newcopy", async: "1", channel: "chunlei", web: "1", app_id: APP_ID,
                bdstoken: this.bdsToken, logid: this.logId, clienttype: "0",
                "dp-logid": new DpLogId().getDpLogId(),
            },
            form: { fsidlist: "[" + info.file_id + "]", path: "/" },
            headers: {
                "Connection": "keep-alive", "Origin": "https://pan.baidu.com",
                "Referer": "https://pan.baidu.com/disk/main", "User-Agent": UA_WEB,
            },
        });
        if (resp.status !== 200) return null;
        try {
            const doc = resp.json();
            if (doc.errno === 0 && doc.extra && Array.isArray(doc.extra.list)) {
                const out = [];
                for (const item of doc.extra.list) {
                    if (item && typeof item.to === "string" && typeof item.to_fs_id === "number") {
                        out.push({
                            file_id: String(item.from_fs_id),
                            path: item.to,
                            name: "", size: 0, is_dir: false, create_time: 0, root_path: "",
                        });
                    }
                }
                return out;
            }
        } catch (e) {
            gdl.log.warn("transferShareFile parse failed: " + e);
        }
        return null;
    }

    // 对应 DeleteTransferShareFile：清理转存文件（best effort）
    async deleteTransferShareFile(info) {
        if (!this.bduss) return false;
        const dataObj = { list: [{ path: info.path }] };
        try {
            const resp = await gdl.http.post("https://pcs.baidu.com/rest/2.0/pcs/file", {
                params: { method: "delete", app_id: "266719" },
                multipart: { param: JSON.stringify(dataObj) },
                headers: { "Cookie": "BDUSS=" + this.bduss },
                use_cookie_jar: false,
            });
            return resp.status === 200;
        } catch (e) {
            return false;
        }
    }

    // 对应 GetDownloadInfo
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

        if (!this.bduss) {
            gdl.notify("Baidu download requires login (BDUSS cookie)", "error");
            return null;
        }
        const transferred = await this.transferShareFile(info);
        if (!transferred) return null;

        const result = [];
        for (const item of transferred) {
            const fileName = item.path.split("/").pop();
            const urls = await getRealDownloadAddress(this.bduss, item.path, this.pcsState);
            if (urls.length === 0) continue;
            const realUrl = urls[0];
            if (!realUrl) continue;

            // 注意：bridge（NetWork_Disk_magager.cxx:105）只读取 ParseResult.headers 并整体作为
            // aria2 选项，忽略 options 字段；out 由 bridge 从 file_name 自动补。故所有 aria2 选项
            // 与请求头都用 aria2 选项名放进 headers。
            const headers = {
                "header": "Cookie:BDUSS=" + this.bduss,
                "user-agent": CLIENT_UA,
            };
            // 非会员：开启分段并发加速（对应 is_vip == "0" 分支）
            if (this.isVip === "0") {
                headers["force-http-range"] = "true";
                headers["enable-http-pipelining"] = "true";
                headers["max-connection-per-server"] = "8";
                headers["split"] = "16";
            }
            result.push({
                real_url: realUrl,
                file_name: fileName,
                file_size: info.size,
                headers: headers,
                options: {},
                // bridge 在 mirrors 非空时用其替代 real_url，故单地址时留空
                mirrors: urls.length > 1 ? urls : [],
            });
            await this.deleteTransferShareFile(item);
        }
        return result;
    }
}
