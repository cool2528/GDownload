// 123云盘分享解析核心流程
// 列目录: GET /b/api/share/get (匿名可用)
// 下载:   POST /b/api/v2/share/download/info (需登录 token + 时间签名) -> 解包套娃直链
// 官方 Open API 不支持解析他人分享,故走网页端 API。协议参考公开的网页接口约定,实现为原创。

const DEFAULT_BASE = "https://www.123pan.com";
// 官方域 www.123pan.com 的网页端 API 已被封(只回 SPA HTML),镜像域仍开放匿名接口;
// shareKey 与域名无关,可在镜像域间回退
const API_HOSTS = [
    "https://www.123912.com", "https://www.123684.com",
    "https://www.123865.com", "https://www.123952.com",
    "https://www.123pan.com",
];
const PLATFORM = "web";
const APP_VERSION = "3";
const UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Chrome/114.0.0.0 Safari/537.36";

// 数字置换表:YYYYMMDDHHmm 每位数字 0-9 映射为对应字母,再求 CRC32
const DIGIT_MAP = "adefghlmyi";

let CRC_TABLE = null;
function crc32(str) {
    if (!CRC_TABLE) {
        CRC_TABLE = new Array(256);
        for (let n = 0; n < 256; n++) {
            let c = n;
            for (let k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
            }
            CRC_TABLE[n] = c >>> 0;
        }
    }
    let crc = 0xFFFFFFFF;
    for (let i = 0; i < str.length; i++) {
        // 签名输入均为 ASCII(数字/字母/竖线/斜杠)
        crc = (crc >>> 8) ^ CRC_TABLE[(crc ^ (str.charCodeAt(i) & 0xFF)) & 0xFF];
    }
    return ((crc ^ 0xFFFFFFFF) >>> 0);
}

function pad2(n) {
    return n < 10 ? "0" + n : String(n);
}

// 生成网页端下载接口所需的时间签名 query 段
// 返回 "?<timeSign>=<timestamp>-<random>-<dataSign>"
function buildSignQuery(path) {
    const timestamp = Math.floor(Date.now() / 1000);
    const random = Math.floor(Math.random() * 10000000);
    // 北京时间(UTC+8) YYYYMMDDHHmm
    const bj = new Date(Date.now() + 8 * 3600 * 1000);
    const ymdhm =
        String(bj.getUTCFullYear()) +
        pad2(bj.getUTCMonth() + 1) +
        pad2(bj.getUTCDate()) +
        pad2(bj.getUTCHours()) +
        pad2(bj.getUTCMinutes());
    let mapped = "";
    for (let i = 0; i < ymdhm.length; i++) {
        mapped += DIGIT_MAP.charAt(ymdhm.charCodeAt(i) - 48);
    }
    const timeSign = crc32(mapped);
    const dataSign = crc32(
        timestamp + "|" + random + "|" + path + "|" + PLATFORM + "|" + APP_VERSION + "|" + timeSign
    );
    return "?" + timeSign + "=" + timestamp + "-" + random + "-" + dataSign;
}

// 大小写兼容取字段
function pick(obj, names) {
    for (const n of names) {
        if (obj && obj[n] !== undefined && obj[n] !== null) return obj[n];
    }
    return undefined;
}

function originOf(url) {
    const m = url.match(/^(https?:\/\/[^/]+)/);
    return m ? m[1] : "";
}

export class Pan123Api {
    constructor() {
        this.reset();
    }

    reset() {
        this.base = "";                       // 命中可用镜像后 pin
        this.hostCandidates = API_HOSTS.slice();
        this.shareKey = "";
        this.sharePwd = "";
        this.token = "";
        // FileId -> { s3, etag, size } 下载接口需要
        this.metaByFid = {};
    }

    commonHeaders(host, extra) {
        const h = host || this.base || DEFAULT_BASE;
        const headers = {
            "User-Agent": UA,
            "Platform": PLATFORM,
            "App-Version": APP_VERSION,
            "Referer": h + "/s/" + this.shareKey + ".html",
            "Origin": h,
        };
        if (this.token) headers["Authorization"] = "Bearer " + this.token;
        return extra ? Object.assign(headers, extra) : headers;
    }

    // 匿名列目录:在镜像域间回退,命中(返回 JSON 且带 code)后 pin this.base
    async shareGet(params) {
        const hosts = this.base ? [this.base] : this.hostCandidates;
        for (const host of hosts) {
            let resp;
            try {
                resp = await gdl.http.get(host + "/b/api/share/get", {
                    params: params,
                    headers: this.commonHeaders(host),
                    timeout: 15000,
                });
            } catch (e) {
                continue;
            }
            if (resp.status !== 200) continue;
            let doc;
            try {
                doc = resp.json();
            } catch (e) {
                continue; // 被封域名返回 SPA HTML,非 JSON
            }
            if (doc && typeof doc.code === "number") {
                this.base = host; // API 可用(无论 code 是否为 0),锁定该镜像
                return doc;
            }
        }
        return null;
    }

    // 从分享链接提取 shareKey(去 .html / query / fragment)
    extractShareKey(url) {
        const m = url.match(/\/s\/([A-Za-z0-9_-]+)/);
        return m ? m[1] : "";
    }

    // 列目录(分页)
    async listDir(parentFileId, basePath) {
        const out = [];
        let page = 1;
        for (; page <= 100; page++) {
            const doc = await this.shareGet({
                shareKey: this.shareKey,
                SharePwd: this.sharePwd || "",
                ParentFileId: parentFileId,
                Page: String(page),
                limit: "100",
                next: "0",
                orderBy: "file_name",
                orderDirection: "asc",
            });
            if (!doc) {
                gdl.log.warn("123 share/get unreachable (all mirrors failed)");
                return out.length ? out : null;
            }
            if (doc.code !== 0) {
                gdl.log.warn("123 share/get error: " + (doc.message || doc.code));
                if (doc.message) gdl.notify(doc.message, "error");
                return out.length ? out : null;
            }
            const data = doc.data || {};
            const list = Array.isArray(data.InfoList) ? data.InfoList : [];
            for (const item of list) {
                const fid = pick(item, ["FileId", "FileID", "fileId"]);
                const fname = pick(item, ["FileName", "fileName"]);
                if (fid === undefined || fname === undefined) continue;
                const typeVal = pick(item, ["Type", "type"]);
                const isDir = Number(typeVal) === 1;
                const size = Number(pick(item, ["Size", "size"]) || 0);
                const fidStr = String(fid);
                this.metaByFid[fidStr] = {
                    s3: String(pick(item, ["S3KeyFlag", "S3keyFlag", "s3KeyFlag"]) || ""),
                    etag: String(pick(item, ["Etag", "etag"]) || ""),
                    size: size,
                };
                out.push({
                    name: String(fname),
                    path: basePath === "/" ? "/" + fname : basePath + "/" + fname,
                    size: size,
                    is_dir: isDir,
                    file_id: fidStr,
                    create_time: 0,
                    root_path: "/",
                });
            }
            const next = pick(data, ["Next", "next"]);
            if (list.length === 0 || String(next) === "-1") break;
        }
        return out;
    }

    // 解包直链套娃:params base64 -> GET 不跟随 -> 302 Location / JSON redirect_url
    async unwrapDownloadUrl(rawUrl) {
        let url = rawUrl;
        // query 里的 params 是 base64 编码的真实中转 URL
        const pm = url.match(/[?&]params=([^&]+)/);
        if (pm) {
            try {
                const decoded = gdl.utils.base64Decode(gdl.utils.urlDecode(pm[1]));
                if (/^https?:\/\//.test(decoded)) url = decoded;
            } catch (e) { /* 保持原 url */ }
        }
        let resp;
        try {
            resp = await gdl.http.get(url, {
                follow_redirects: false,
                headers: { "User-Agent": UA, "Referer": this.base + "/" },
                timeout: 15000,
            });
        } catch (e) {
            gdl.log.warn("123 unwrap failed: " + e);
            return url; // 兜底:直接用当前 url
        }
        if (resp.status === 301 || resp.status === 302) {
            let loc = resp.headers["location"];
            if (Array.isArray(loc)) loc = loc[0];
            if (loc) return loc;
        }
        if (resp.status === 200) {
            try {
                const doc = resp.json();
                const red = pick(doc.data || doc, ["redirect_url", "redirectUrl", "RedirectUrl"]);
                if (red) return String(red);
            } catch (e) { /* 非 JSON,落到兜底 */ }
        }
        return url;
    }

    // 取单文件下载直链
    async fetchDownloadUrl(info) {
        if (!this.token) {
            gdl.notify("123pan download requires a login token (set it in plugin settings)", "error");
            return null;
        }
        const meta = this.metaByFid[info.file_id] || {};
        // 优先 v2 接口,失败回退 v1
        const v2 = await this.requestDownloadInfo("/b/api/v2/share/download/info", info, meta);
        if (v2) return v2;
        return this.requestDownloadInfo("/a/api/share/download/info", info, meta);
    }

    async requestDownloadInfo(path, info, meta) {
        const url = this.base + path + buildSignQuery(path);
        let resp;
        try {
            resp = await gdl.http.post(url, {
                json: {
                    ShareKey: this.shareKey,
                    FileID: Number(info.file_id) || info.file_id,
                    S3keyFlag: meta.s3 || "",
                    Size: meta.size || info.size || 0,
                    Etag: meta.etag || "",
                    SharePwd: this.sharePwd || "",
                },
                headers: this.commonHeaders(this.base, { "Content-Type": "application/json" }),
                timeout: 15000,
            });
        } catch (e) {
            gdl.log.warn("123 download/info failed: " + e);
            return null;
        }
        if (resp.status !== 200) return null;
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            return null;
        }
        if (doc.code !== 0) {
            gdl.log.warn("123 download/info error: " + (doc.message || doc.code));
            if (doc.message) gdl.notify(doc.message, "error");
            return null;
        }
        const data = doc.data || {};
        // v2: downloadPath + dispatchList[0].prefix ; v1: DownloadURL / DownloadUrl
        let rawUrl = "";
        const dpath = pick(data, ["downloadPath", "DownloadPath"]);
        if (dpath) {
            const prefix = Array.isArray(data.dispatchList) && data.dispatchList[0]
                ? pick(data.dispatchList[0], ["prefix", "Prefix"]) : "";
            if (prefix) {
                rawUrl = String(prefix).replace(/\/$/, "") + "/" + String(dpath).replace(/^\//, "");
            }
        }
        if (!rawUrl) {
            rawUrl = String(pick(data, ["DownloadURL", "DownloadUrl", "downloadUrl"]) || "");
        }
        if (!rawUrl) return null;

        const finalUrl = await this.unwrapDownloadUrl(rawUrl);
        return {
            real_url: finalUrl,
            file_name: info.name,
            file_size: info.size || meta.size || 0,
            headers: {
                "header": "Referer:" + this.base + "/",
                "user-agent": UA,
            },
            options: {},
            mirrors: [],
        };
    }

    // ---- 对外主流程 ----

    async parseUrl(url, userToken) {
        gdl.log.info("123 parse started");
        this.reset();
        this.token = (userToken || "").trim().replace(/^bearer\s+/i, "");

        // 候选 API 域:用户粘贴的域优先(若是 123 域),再镜像域回退
        const origin = originOf(url);
        if (origin && /123/.test(origin) && this.hostCandidates.indexOf(origin) < 0) {
            this.hostCandidates.unshift(origin);
        }

        this.shareKey = this.extractShareKey(url);
        if (!this.shareKey) {
            gdl.log.warn("123 invalid share url");
            return null;
        }
        const pm = url.match(/[?&#](?:pwd|SharePwd|passcode)=([^&\s]+)/);
        this.sharePwd = pm ? gdl.utils.urlDecode(pm[1]) : "";

        const files = await this.listDir("0", "/");
        if (!files) {
            gdl.log.warn("123 share list failed");
            return null;
        }
        gdl.log.info("123 parse completed files=" + files.length);
        return files;
    }

    async enterDirectory(info) {
        if (!this.shareKey) {
            gdl.log.warn("123 enterDirectory without session");
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
