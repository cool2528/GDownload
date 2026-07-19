// 蓝奏云分享解析核心流程(全程无需登录/cookie)
// 单文件: GET 分享页(去注释) -> [无密码]取 iframe/fn 页 / [有密码]取 down_p 参数
//         -> POST ajaxm.php 得 dom+url -> GET dom/file/url 不跟随取 302 直链
// 文件夹: GET 分享页 -> POST filemoreajax.php 分页列文件 -> 每个文件走单文件流程
// 协议参考公开的 ajaxm.php / filemoreajax.php 调用约定,实现为原创。

const UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " +
    "Chrome/114.0.0.0 Safari/537.36";
const ACCEPT_LANG = "zh-CN,zh;q=0.9";

// 蓝奏云可用镜像二级域(主域常不可达),解析前逐个试探
const MIRRORS = [
    "lanzouo.com", "lanzoui.com", "lanzoux.com",
    "lanzouw.com", "lanzoup.com", "lanzn.com",
];

// 去除 HTML/JS 注释:页面会在注释里埋假的 sign/iframe/var 误导解析
function stripComments(html) {
    let s = html.replace(/<!--[\s\S]*?-->/g, "");
    s = s.replace(/\/\*[\s\S]*?\*\//g, "");
    // 行注释:仅当 // 前不是冒号(避免误伤 http:// / https://)
    s = s.replace(/([^:])\/\/[^\n\r]*/g, "$1");
    return s;
}

// 解析值:若像变量名(短标识符),回查 var name='value'(取最后一次赋值)
function resolveValue(html, raw) {
    if (raw == null) return "";
    let v = String(raw).trim().replace(/^['"]|['"]$/g, "").trim();
    if (/^[A-Za-z_$][\w$]*$/.test(v) && v.length < 20) {
        const re = new RegExp("var\\s+" + v + "\\s*=\\s*['\"]([^'\"]*)['\"]", "g");
        let m, last = null;
        while ((m = re.exec(html)) !== null) last = m[1];
        if (last !== null) return last;
    }
    return v;
}

// 从一段 JS 里提取传给 $.ajax 的 data:{...} 对象(键->原始值,值可能是变量名)
function extractDataObject(js) {
    const m = js.match(/data\s*:\s*\{([\s\S]*?)\}/);
    if (!m) return null;
    const body = m[1];
    const obj = {};
    const pairRe = /['"]?([A-Za-z_]\w*)['"]?\s*:\s*([^,}]+)/g;
    let p;
    while ((p = pairRe.exec(body)) !== null) {
        obj[p[1]] = p[2].trim();
    }
    return obj;
}

// 取原始 URL 的 origin(scheme+host)
function originOf(url) {
    const m = url.match(/^(https?:\/\/[^/]+)/);
    return m ? m[1] : "";
}

export class LanzouApi {
    constructor() {
        this.reset();
    }

    reset() {
        this.base = "";        // 实际可用的 origin,如 https://xxx.lanzoui.com
        this.pageUrl = "";     // 实际可用的分享页完整 URL(Referer 用)
        this.pwdByUrl = {};    // 单文件分享 URL -> 提取码
    }

    // 生成候选 URL:原始 host 优先,再逐个镜像(保留子域,并补 www / 裸域)
    buildCandidates(url) {
        const m = url.match(/^https?:\/\/([^/]+)(\/.*)$/);
        if (!m) return [url];
        const host = m[1];
        const path = m[2];
        const parts = host.split(".");
        const sub = parts.length > 2 ? parts.slice(0, parts.length - 2).join(".") : "www";
        const out = [];
        const seen = {};
        const add = (h) => {
            const u = "https://" + h + path;
            if (!seen[u]) { seen[u] = 1; out.push(u); }
        };
        add(host);
        for (const mir of MIRRORS) {
            add(sub + "." + mir);
            add("www." + mir);
        }
        return out;
    }

    // 取分享页 HTML:先试给定 URL,失败再遍历镜像候选
    async fetchPage(url) {
        const candidates = this.buildCandidates(url);
        for (const cand of candidates) {
            let resp;
            try {
                resp = await gdl.http.get(cand, {
                    headers: { "User-Agent": UA, "Accept-Language": ACCEPT_LANG },
                    timeout: 12000,
                });
            } catch (e) {
                continue;
            }
            if (resp.status !== 200) continue;
            const html = resp.text();
            // 合理性判断:命中蓝奏云页面特征
            if (/lanzou|蓝奏|ajaxm\.php|filemoreajax|pwdload|iframe/i.test(html)) {
                return { html: stripComments(html), url: cand };
            }
        }
        return null;
    }

    // 判别文件夹分享:页面含 filemoreajax 调用
    isFolderPage(html) {
        return /filemoreajax\.php/.test(html);
    }

    // 从分享页提取文件名(浏览展示用;权威文件名以 ajaxm 的 inf 为准)
    extractFileName(html) {
        let m = html.match(/<div class="[^"]*n_box_3fn[^"]*"[^>]*>([^<]+)</);
        if (m) return m[1].trim();
        m = html.match(/<title>([^<]+)<\/title>/);
        if (m) {
            return m[1].replace(/\s*-\s*(蓝奏云|蓝奏网盘|Lanzou).*$/i, "").trim();
        }
        m = html.match(/var\s+filename\s*=\s*['"]([^'"]+)['"]/);
        if (m) return m[1].trim();
        return "";
    }

    // ---- 单文件解析 ----

    // 从分享页解析出 ajaxm.php 所需的 fileID 与 form 参数
    async buildAjaxRequest(page, pwd) {
        const html = page.html;
        const base = originOf(page.url);
        const hasPwd = /pwdload|passwddiv/.test(html);

        let dataJs = html;       // 提取 data 对象的 JS 源(无密码时来自 iframe 页)
        let referer = page.url;

        if (!hasPwd) {
            // 无密码:页面有 iframe,其 src 指向 /fn? 下载中间页
            const im = html.match(/<iframe[^>]*\ssrc="([^"]+)"/i);
            if (!im) {
                gdl.log.warn("lanzou iframe not found");
                return null;
            }
            const iframeUrl = im[1].charAt(0) === "/" ? base + im[1] : im[1];
            let ifResp;
            try {
                ifResp = await gdl.http.get(iframeUrl, {
                    headers: {
                        "User-Agent": UA, "Accept-Language": ACCEPT_LANG,
                        "Referer": page.url,
                    },
                });
            } catch (e) {
                gdl.log.warn("lanzou iframe fetch failed: " + e);
                return null;
            }
            if (ifResp.status !== 200) return null;
            dataJs = stripComments(ifResp.text());
            referer = iframeUrl;
        }

        const rawObj = extractDataObject(dataJs);
        if (!rawObj) {
            gdl.log.warn("lanzou ajax data object not found");
            return null;
        }
        // 逐值解析(处理变量间接引用)
        const form = {};
        for (const k of Object.keys(rawObj)) {
            form[k] = resolveValue(dataJs, rawObj[k]);
        }
        if (!form.action) form.action = "downprocess";
        if (hasPwd) form.p = pwd || "";

        // fileID:新版接口带 ajaxm.php?file=<id>
        let fileId = "";
        const fm = dataJs.match(/ajaxm\.php\?file=(\d+)/) || html.match(/ajaxm\.php\?file=(\d+)/);
        if (fm) fileId = fm[1];

        return { base, form, referer, fileId };
    }

    // 解析单文件真实下载直链
    async resolveDownload(shareUrl, pwd, knownSize, knownName) {
        const page = await this.fetchPage(shareUrl);
        if (!page) {
            gdl.log.warn("lanzou file page unreachable");
            return null;
        }
        const req = await this.buildAjaxRequest(page, pwd);
        if (!req) return null;

        const ajaxUrl = req.base + "/ajaxm.php" + (req.fileId ? "?file=" + req.fileId : "");
        let resp;
        try {
            resp = await gdl.http.post(ajaxUrl, {
                form: req.form,
                headers: {
                    "User-Agent": UA, "Accept-Language": ACCEPT_LANG,
                    "Referer": req.referer, "X-Requested-With": "XMLHttpRequest",
                },
            });
        } catch (e) {
            gdl.log.warn("lanzou ajaxm failed: " + e);
            return null;
        }
        if (resp.status !== 200) {
            gdl.log.warn("lanzou ajaxm http " + resp.status);
            return null;
        }
        let doc;
        try {
            doc = resp.json();
        } catch (e) {
            gdl.log.warn("lanzou ajaxm parse failed: " + e);
            return null;
        }
        // zt / inf 类型不定(string|number),宽松判断
        if (String(doc.zt) !== "1" || !doc.dom || !doc.url) {
            gdl.log.warn("lanzou ajaxm error: " + (doc.inf != null ? doc.inf : doc.zt));
            return null;
        }
        const fileName = (knownName || (typeof doc.inf === "string" ? doc.inf : "") || "")
            .replace(/\*/g, "_");
        const relayUrl = String(doc.dom).replace(/\/$/, "") + "/file/" + doc.url;

        const realUrl = await this.followRelay(relayUrl);
        if (!realUrl) return null;

        return {
            real_url: realUrl,
            file_name: fileName || "lanzou_file",
            file_size: knownSize || 0,
            headers: { "user-agent": UA },
            options: {},
            mirrors: [],
        };
    }

    // 中转地址 -> 最终 CDN 直链:不跟随重定向,取 302 Location;处理"网络异常"二次验证
    async followRelay(relayUrl) {
        let resp;
        try {
            resp = await gdl.http.get(relayUrl, {
                follow_redirects: false,
                headers: {
                    "User-Agent": UA, "Accept-Language": ACCEPT_LANG,
                    "Cookie": "down_ip=1",
                },
            });
        } catch (e) {
            gdl.log.warn("lanzou relay failed: " + e);
            return null;
        }
        if (resp.status === 302 || resp.status === 301) {
            let loc = resp.headers["location"];
            if (Array.isArray(loc)) loc = loc[0];
            if (loc) return loc;
        }
        if (resp.status === 200) {
            const body = resp.text();
            // "网络异常"二次验证:提取 file/sign,等待 >=2s,POST <dom>/file/ajax.php el=2
            const fm = body.match(/['"]file['"]\s*:\s*['"]([^'"]+)['"]/);
            const sm = body.match(/['"]sign['"]\s*:\s*['"]([^'"]+)['"]/);
            if (fm && sm) {
                await gdl.utils.sleep(2200);
                const ajaxUrl = originOf(relayUrl) + "/file/ajax.php";
                try {
                    const vr = await gdl.http.post(ajaxUrl, {
                        form: { file: fm[1], sign: sm[1], el: "2" },
                        headers: {
                            "User-Agent": UA, "Accept-Language": ACCEPT_LANG,
                            "Referer": relayUrl, "X-Requested-With": "XMLHttpRequest",
                        },
                    });
                    if (vr.status === 200) {
                        const vd = vr.json();
                        if (vd && vd.url) return vd.url;
                    }
                } catch (e) {
                    gdl.log.warn("lanzou relay verify failed: " + e);
                }
            }
            gdl.log.warn("lanzou relay returned challenge page");
        }
        return null;
    }

    // ---- 文件夹解析 ----

    async parseFolder(folderUrl, pwd) {
        const page = await this.fetchPage(folderUrl);
        if (!page) {
            gdl.log.warn("lanzou folder page unreachable");
            return null;
        }
        const html = page.html;
        const base = originOf(page.url);
        this.base = base;
        this.pageUrl = page.url;

        const rawObj = extractDataObject(html);
        if (!rawObj) {
            gdl.log.warn("lanzou folder ajax data not found");
            return null;
        }
        const baseForm = {};
        for (const k of Object.keys(rawObj)) {
            baseForm[k] = resolveValue(html, rawObj[k]);
        }
        if (pwd) baseForm.pwd = pwd;

        const out = [];
        for (let pg = 1; pg <= 100; pg++) {
            const form = Object.assign({}, baseForm, { pg: String(pg) });
            let resp;
            try {
                resp = await gdl.http.post(base + "/filemoreajax.php", {
                    form: form,
                    headers: {
                        "User-Agent": UA, "Accept-Language": ACCEPT_LANG,
                        "Referer": page.url, "X-Requested-With": "XMLHttpRequest",
                    },
                });
            } catch (e) {
                gdl.log.warn("lanzou filemoreajax failed: " + e);
                break;
            }
            if (resp.status !== 200) break;
            let doc;
            try {
                doc = resp.json();
            } catch (e) {
                break;
            }
            const zt = String(doc.zt);
            if (zt === "3") {
                gdl.log.warn("lanzou folder password error");
                return null;
            }
            const list = Array.isArray(doc.text) ? doc.text : [];
            for (const item of list) {
                if (!item || typeof item.id !== "string") continue;
                const name = typeof item.name_all === "string" ? item.name_all : (item.name || "");
                const fileShareUrl = base + "/" + item.id;
                this.pwdByUrl[fileShareUrl] = ""; // 文件夹内文件通常无独立密码
                out.push({
                    name: name,
                    path: "/" + name,
                    size: this.parseSize(item.size),
                    is_dir: false,
                    file_id: fileShareUrl,
                    create_time: 0,
                    root_path: "/",
                });
            }
            if (zt === "2" || list.length === 0) break;
            await gdl.utils.sleep(1000); // 分页限频
        }
        return out;
    }

    // 蓝奏云 size 是 "1.5 M" 之类字符串,转字节(近似;失败返回 0)
    parseSize(s) {
        if (typeof s === "number") return s;
        if (typeof s !== "string") return 0;
        const m = s.trim().match(/([\d.]+)\s*([KMGT]?)/i);
        if (!m) return 0;
        const n = parseFloat(m[1]);
        const unit = m[2].toUpperCase();
        const mult = { "": 1, K: 1024, M: 1048576, G: 1073741824, T: 1099511627776 }[unit] || 1;
        return Math.round(n * mult);
    }

    // ---- 对外主流程 ----

    async parseUrl(url, userToken) {
        gdl.log.info("lanzou parse started");
        this.reset();

        // 提取码:URL 上无统一约定,兼容用户手动追加 ?pwd= / &pwd=
        const pm = url.match(/[?&#]pwd=([^&\s]+)/);
        const pwd = pm ? gdl.utils.urlDecode(pm[1]) : "";
        // 去掉自定义 pwd 参数再解析(避免污染蓝奏原生参数)
        const cleanUrl = url.replace(/([?&#])pwd=[^&\s]*/g, "$1").replace(/[?&#]+$/, "");

        const page = await this.fetchPage(cleanUrl);
        if (!page) {
            gdl.log.warn("lanzou share page unreachable");
            return null;
        }
        this.base = originOf(page.url);
        this.pageUrl = page.url;

        if (this.isFolderPage(page.html)) {
            const files = await this.parseFolder(page.url, pwd);
            if (!files) return null;
            gdl.log.info("lanzou folder parsed files=" + files.length);
            return files;
        }

        // 单文件:返回一条 FileInfo,直链解析推迟到 getDownloadInfo(直链短时效)
        const name = this.extractFileName(page.html);
        this.pwdByUrl[page.url] = pwd;
        gdl.log.info("lanzou single file parsed");
        return [{
            name: name || "lanzou_file",
            path: "/" + (name || "lanzou_file"),
            size: 0,
            is_dir: false,
            file_id: page.url,
            create_time: 0,
            root_path: "/",
        }];
    }

    async enterDirectory(info) {
        // 子文件夹:file_id 为子文件夹分享 URL
        return this.parseFolder(info.file_id, this.pwdByUrl[info.file_id] || "");
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
        const pwd = this.pwdByUrl[info.file_id] || "";
        const result = await this.resolveDownload(info.file_id, pwd, info.size, info.name);
        return result ? [result] : null;
    }
}
