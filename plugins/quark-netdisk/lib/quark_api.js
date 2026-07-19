// 夸克网盘分享解析核心流程
// 分享文件无法直接下载,须先转存到登录账号网盘再取直链:
//   fetchStoken -> listDir -> saveAndWait -> fetchDownloadUrl -> (deleteFile)
// 协议参考夸克 sharepage / clouddrive API 的公开调用约定,实现为原创。

const API_HOST = "https://drive-pc.quark.cn";
const PAN_HOST = "https://pan.quark.cn";

// 直链下载与 API 均需夸克 PC 客户端 UA,否则触发限速/403
const CLIENT_UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "quark-cloud-drive/2.5.20 Chrome/100.0.4896.160 Electron/18.3.5.4-b478491100 " +
    "Safari/537.36 Channel/pckk_other_ch";

// 从 Set-Cookie 响应头(数组或字符串)中提取指定 cookie 值
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

// 在 cookie 串中更新或追加一个键值(用于滚动刷新 __puus / __pus)
function upsertCookie(cookieStr, name, value) {
    const re = new RegExp("(^|;\\s*)" + name + "=[^;]*");
    if (re.test(cookieStr)) {
        return cookieStr.replace(re, "$1" + name + "=" + value);
    }
    return cookieStr ? cookieStr + "; " + name + "=" + value : name + "=" + value;
}

// 防缓存参数:对齐官方客户端行为,降低风控概率
function antiCacheParams() {
    const rnd = Math.floor(Math.random() * 900000000) + 100000000;
    return { __dt: String(rnd), __t: String(Date.now()) };
}

export class QuarkApi {
    constructor() {
        this.reset();
    }

    reset() {
        this.cookie = "";
        this.pwdId = "";
        this.passcode = "";
        this.stoken = "";
        // fid -> share_fid_token(转存必需,逐文件),fid -> 分享内父目录 fid
        this.tokenByFid = {};
        this.parentByFid = {};
    }

    // 统一请求:自动附带公共 query、客户端头、显式 Cookie;响应后滚动刷新 Cookie
    async request(method, path, opts) {
        opts = opts || {};
        const params = Object.assign({ pr: "ucpro", fr: "pc" }, opts.params || {});
        const headers = {
            "User-Agent": CLIENT_UA,
            "Referer": PAN_HOST + "/",
            "Origin": PAN_HOST,
            "Accept": "application/json, text/plain, */*",
        };
        if (this.cookie) headers["Cookie"] = this.cookie;
        const reqOpts = { params, headers, use_cookie_jar: false, timeout: 15000 };
        if (opts.json !== undefined) reqOpts.json = opts.json;

        let resp;
        if (method === "POST") {
            resp = await gdl.http.post(API_HOST + path, reqOpts);
        } else {
            resp = await gdl.http.get(API_HOST + path, reqOpts);
        }
        this.refreshCookie(resp.headers);
        return resp;
    }

    // 夸克会周期性通过 Set-Cookie 下发新的 __puus / __pus,不跟随更新会导致 Cookie 失效
    refreshCookie(headers) {
        for (const name of ["__puus", "__pus"]) {
            const v = extractSetCookieValue(headers, name);
            if (v) this.cookie = upsertCookie(this.cookie, name, v);
        }
    }

    // 从 https://pan.quark.cn/s/<pwd_id> 提取分享 id 与提取码
    extractShareId(url) {
        const clean = url.split("#")[0];
        const m = clean.match(/\/s\/([A-Za-z0-9]+)/);
        const pwdId = m ? m[1] : "";
        // 提取码在 URL 上无统一约定,兼容 pwd / passcode 两种参数
        const pm = clean.match(/[?&](?:pwd|passcode)=([^&]+)/);
        const passcode = pm ? gdl.utils.urlDecode(pm[1]) : "";
        return { pwdId, passcode };
    }

    // 解析响应包裹:成功须 status==200 且 code==0
    parseEnvelope(resp, stage) {
        if (resp.status !== 200) {
            gdl.log.warn("quark " + stage + " http " + resp.status);
            return null;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            gdl.log.warn("quark " + stage + " parse failed: " + e);
            return null;
        }
        if (doc.code !== 0) {
            gdl.log.warn("quark " + stage + " error: " + (doc.message || doc.code));
            if (doc.message) gdl.notify(doc.message, "error");
            return null;
        }
        return doc;
    }

    // 获取分享 token(后续所有分享操作都要带 stoken)
    async fetchStoken() {
        const resp = await this.request("POST", "/1/clouddrive/share/sharepage/token", {
            json: { pwd_id: this.pwdId, passcode: this.passcode || "" },
        });
        const doc = this.parseEnvelope(resp, "token");
        if (!doc || !doc.data || typeof doc.data.stoken !== "string" || !doc.data.stoken) {
            return false;
        }
        this.stoken = doc.data.stoken;
        return true;
    }

    // 列目录(分页聚合);pdirFid 根目录为 "0",子目录为其 fid
    async listDir(pdirFid, basePath) {
        const out = [];
        let page = 1;
        const size = 50;
        for (;;) {
            const resp = await this.request("GET", "/1/clouddrive/share/sharepage/detail", {
                params: {
                    pwd_id: this.pwdId,
                    stoken: this.stoken, // 由 http 层负责 query 编码
                    pdir_fid: pdirFid,
                    force: "0",
                    _page: String(page),
                    _size: String(size),
                    _fetch_banner: "0",
                    _fetch_share: "0",
                    _fetch_total: "1",
                    _sort: "file_type:asc,updated_at:desc",
                },
            });
            const doc = this.parseEnvelope(resp, "detail");
            if (!doc || !doc.data) return out.length ? out : null;

            const list = Array.isArray(doc.data.list) ? doc.data.list : [];
            for (const item of list) {
                if (typeof item.fid !== "string" || typeof item.file_name !== "string") continue;
                const name = item.file_name;
                const file = {
                    name: name,
                    path: basePath === "/" ? "/" + name : basePath + "/" + name,
                    size: typeof item.size === "number" ? item.size : 0,
                    is_dir: item.dir === true,
                    file_id: item.fid,
                    create_time: typeof item.updated_at === "number"
                        ? Math.floor(item.updated_at / 1000) : 0,
                    root_path: "/",
                };
                this.tokenByFid[item.fid] =
                    typeof item.share_fid_token === "string" ? item.share_fid_token : "";
                this.parentByFid[item.fid] = pdirFid;
                out.push(file);
            }

            const total = doc.metadata && typeof doc.metadata._total === "number"
                ? doc.metadata._total : out.length;
            if (list.length === 0 || out.length >= total) break;
            page += 1;
            if (page > 200) break; // 兜底:防异常分页导致死循环
        }
        return out;
    }

    // 转存分享文件到登录账号网盘,轮询任务完成后返回新 fid
    async saveAndWait(fid, fidToken, pdirFid) {
        const resp = await this.request("POST", "/1/clouddrive/share/sharepage/save", {
            params: Object.assign({ app: "clouddrive" }, antiCacheParams()),
            json: {
                fid_list: [fid],
                fid_token_list: [fidToken],
                to_pdir_fid: "0",
                pwd_id: this.pwdId,
                stoken: this.stoken,
                pdir_fid: pdirFid,
                scene: "link",
            },
        });
        const doc = this.parseEnvelope(resp, "save");
        if (!doc || !doc.data || !doc.data.task_id) return null;
        return this.pollTask(doc.data.task_id);
    }

    // 轮询转存任务:status==2 完成,==3 失败;完成后取 save_as.save_as_top_fids[0]
    async pollTask(taskId) {
        for (let retry = 0; retry < 30; retry++) {
            const resp = await this.request("GET", "/1/clouddrive/task", {
                params: Object.assign(
                    { task_id: taskId, retry_index: String(retry) },
                    antiCacheParams()
                ),
            });
            const doc = this.parseEnvelope(resp, "task");
            if (!doc || !doc.data) return null;
            const status = doc.data.status;
            if (status === 2) {
                const fids = doc.data.save_as && doc.data.save_as.save_as_top_fids;
                return Array.isArray(fids) && fids.length ? fids[0] : null;
            }
            if (status === 3) {
                gdl.log.warn("quark save task failed");
                return null;
            }
            await gdl.utils.sleep(800);
        }
        gdl.log.warn("quark save task timeout");
        return null;
    }

    // 取转存后文件的真实下载直链
    async fetchDownloadUrl(fid) {
        const resp = await this.request("POST", "/1/clouddrive/file/download", {
            json: { fids: [fid] },
        });
        const doc = this.parseEnvelope(resp, "download");
        if (!doc || !Array.isArray(doc.data) || !doc.data[0]) return null;
        const url = doc.data[0].download_url;
        return typeof url === "string" && url ? url : null;
    }

    // 清理转存文件(best effort,失败不影响下载)
    async deleteFile(fid) {
        try {
            await this.request("POST", "/1/clouddrive/file/delete", {
                json: { action_type: 2, filelist: [fid], exclude_fids: [] },
            });
        } catch (e) {
            gdl.log.warn("quark cleanup failed: " + e);
        }
        return true;
    }

    // ---- 对外主流程 ----

    async parseUrl(url, userToken) {
        gdl.log.info("quark parse started");
        this.reset();
        this.cookie = (userToken || "").trim();

        const ids = this.extractShareId(url);
        if (!ids.pwdId) {
            gdl.log.warn("invalid quark share url");
            return null;
        }
        this.pwdId = ids.pwdId;
        this.passcode = ids.passcode;

        if (!(await this.fetchStoken())) {
            gdl.log.warn("quark stoken failed");
            return null;
        }
        const files = await this.listDir("0", "/");
        if (!files) {
            gdl.log.warn("quark share list failed");
            return null;
        }
        gdl.log.info("quark parse completed files=" + files.length);
        return files;
    }

    async enterDirectory(info) {
        if (!this.stoken) {
            gdl.log.warn("quark enterDirectory without session");
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

        if (!this.cookie) {
            gdl.notify("Quark download requires login cookie", "error");
            return null;
        }

        // 用户可关闭自动清理,默认开启(与参考实现一致:直链为预签名 CDN 地址,转存后清理不影响下载)
        const autoClean = gdl.config.get("auto_clean") !== false;

        const fidToken = this.tokenByFid[info.file_id] || "";
        const pdir = this.parentByFid[info.file_id] || "0";
        const savedFid = await this.saveAndWait(info.file_id, fidToken, pdir);
        if (!savedFid) {
            gdl.log.warn("quark save/transfer failed");
            return null;
        }

        const realUrl = await this.fetchDownloadUrl(savedFid);
        if (!realUrl) {
            if (autoClean) await this.deleteFile(savedFid);
            return null;
        }

        // 直链下载必须带 Cookie + 客户端 UA + Referer,否则 403
        const result = [{
            real_url: realUrl,
            file_name: info.name,
            file_size: info.size,
            headers: {
                "header": ["Cookie:" + this.cookie, "Referer:" + PAN_HOST + "/"],
                "user-agent": CLIENT_UA,
            },
            options: {},
            mirrors: [],
        }];

        if (autoClean) await this.deleteFile(savedFid);
        return result;
    }
}
