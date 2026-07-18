// 百度 PCS 用户信息与真实下载地址解析（对应 baidu_user.h::BaiduPCS 与 GetRealDownloadAddress）
import { PHONE_MODELS } from "./phone_models.js";
import { quotePlus, nowSeconds } from "./util.js";

const CLIENT_UA =
    "netdisk;P2SP;3.0.0.8;netdisk;11.12.3;ANG-AN00;android-android;10.0;JSbridge4.4.0;jointBridge;1.1.0;";
const PCS_BAIDU = "https://pcs.baidu.com";
const PAN_APP_ID = "250528";

// 对应 sum_IMEI：64 位哈希，需用 BigInt 保证精度
function sumIMEI(key) {
    let hs = 53202347234687234n;
    for (let i = 0; i < key.length; i++) {
        hs += (hs << 5n) + BigInt(key.charCodeAt(i));
    }
    hs %= 1000000000000000n; // 1e15
    if (hs < 100000000000000n) {
        hs += 100000000000000n; // 1e14
    }
    return hs.toString();
}

// 对应 get_phone_model：32 位无符号哈希取模选型号
function getPhoneModel(key) {
    if (PHONE_MODELS.length === 0) {
        return "S3";
    }
    let hs = 2134;
    for (let i = 0; i < key.length; i++) {
        hs = (hs + ((hs << 4) + key.charCodeAt(i))) >>> 0;
    }
    return PHONE_MODELS[hs % PHONE_MODELS.length];
}

// 对应 BaiduPCS::InitUserInfo：通过 tieba 登录接口换取 uid
// 返回 uid 字符串，失败返回 ""
export async function initUserId(bduss) {
    const timestamp = String(nowSeconds());
    const model = getPhoneModel(bduss);
    const phoneImei = sumIMEI(bduss);

    const data = {
        bdusstoken: bduss + "|null",
        channel_id: "",
        channel_uid: "",
        stErrorNums: "0",
        subapp_type: "mini",
        timestamp: timestamp + "922",
        _client_type: "2",
        _client_version: "7.0.0.0",
        _phone_imei: phoneImei,
        from: "mini_ad_wandoujia",
    };
    data["model"] = model;

    const cuidBase = bduss + "_" + data._client_version + "_" + data._phone_imei + "_" + data.from;
    let cuidMd5 = gdl.crypto.md5(cuidBase).toUpperCase();
    const reversedImei = phoneImei.split("").reverse().join("");
    data["cuid"] = cuidMd5 + "|" + reversedImei;

    // 按 key 排序拼接 key=value，追加盐后 MD5 大写
    const keys = Object.keys(data).sort();
    let signBase = "";
    for (const key of keys) {
        signBase += key + "=" + data[key];
    }
    signBase += "tiebaclient!!!";
    data["sign"] = gdl.crypto.md5(signBase).toUpperCase();

    const resp = await gdl.http.post("https://tieba.baidu.com/c/s/login", {
        form: data,
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
            "Cookie": "ka=open",
            "net": "1",
            "User-Agent": "bdtb for Android 6.9.2.1",
            "client_logid": timestamp + "416",
            "Connection": "Keep-Alive",
        },
        use_cookie_jar: false,
    });

    if (resp.status !== 200) {
        return "";
    }
    try {
        const info = resp.json();
        if (info.user && typeof info.user.id !== "undefined") {
            return String(info.user.id);
        }
    } catch (e) {
        gdl.log.warn("initUserId parse failed: " + e);
    }
    return "";
}

// 对应 GetRealDownloadAddress：pcs locatedownload 取真实直链
// pcsState: { uid } 缓存，避免重复登录
export async function getRealDownloadAddress(bduss, filePath, pcsState) {
    if (!pcsState.uid) {
        pcsState.uid = await initUserId(bduss);
        if (!pcsState.uid) {
            gdl.log.warn("pcs uid unavailable");
            return [];
        }
    }
    const uid = pcsState.uid;
    let devuid = gdl.crypto.md5(bduss).toUpperCase() + "|0";
    const enc = gdl.crypto.sha1(bduss);
    const timestamp = String(nowSeconds());
    const randBase = enc + uid + "ebrcUYiuxaZv2XGu7KIYKxUrqfnOfpDF" + timestamp + devuid;
    const rand = gdl.crypto.sha1(randBase);

    const params = {
        apn_id: "1_0",
        app_id: PAN_APP_ID,
        channel: "0",
        check_blue: "1",
        clienttype: "17",
        es: "1",
        esl: "1",
        freeisp: "0",
        method: "locatedownload",
        path: quotePlus(filePath),
        queryfree: "0",
        use: "0",
        ver: "4.0",
        time: timestamp,
    };
    // 与 C++ 版一致：按插入序拼接 query（map 有序，这里显式保持相同顺序）
    let paramsStr = "";
    const order = ["apn_id", "app_id", "channel", "check_blue", "clienttype", "es", "esl",
        "freeisp", "method", "path", "queryfree", "use", "ver", "time"];
    // 与 C++ std::map 一致按字典序
    const sortedKeys = order.slice().sort();
    for (const k of sortedKeys) {
        if (paramsStr) paramsStr += "&";
        paramsStr += k + "=" + params[k];
    }

    let url = PCS_BAIDU + "/rest/2.0/pcs/file?" + paramsStr;
    url += "&rand=" + rand + "&devuid=" + devuid + "&cuid=" + devuid;

    const resp = await gdl.http.get(url, {
        headers: { "User-Agent": CLIENT_UA, "Connection": "Keep-Alive", "Cookie": "BDUSS=" + bduss },
        use_cookie_jar: false,
    });
    if (resp.status !== 200) {
        return [];
    }
    try {
        const info = resp.json();
        const urls = [];
        if (Array.isArray(info.urls)) {
            for (const item of info.urls) {
                if (item && typeof item.url === "string") {
                    urls.push(item.url);
                }
            }
        } else if (info.urls && typeof info.urls.url === "string") {
            urls.push(info.urls.url);
        }
        return urls;
    } catch (e) {
        gdl.log.warn("getRealDownloadAddress parse failed: " + e);
        return [];
    }
}

export { CLIENT_UA };
