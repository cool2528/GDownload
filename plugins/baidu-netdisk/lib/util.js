// 通用工具：对应 C++ 版 detail 命名空间与若干静态辅助函数

// Python quote_plus 语义：unreserved 保留，空格转 +，其余 %XX 大写
// 对应 baidu_user.h::quote_plus
export function quotePlus(value) {
    let out = "";
    for (let i = 0; i < value.length; i++) {
        const c = value.charCodeAt(i);
        const ch = value[i];
        const isAlnum = (c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a);
        if (isAlnum || ch === "-" || ch === "_" || ch === "." || ch === "~") {
            out += ch;
        } else if (ch === " ") {
            out += "+";
        } else {
            // 按 UTF-8 字节百分号编码
            const bytes = utf8Bytes(ch);
            for (const b of bytes) {
                out += "%" + b.toString(16).toUpperCase().padStart(2, "0");
            }
        }
    }
    return out;
}

function utf8Bytes(str) {
    const bytes = [];
    for (let i = 0; i < str.length; i++) {
        let code = str.charCodeAt(i);
        if (code < 0x80) {
            bytes.push(code);
        } else if (code < 0x800) {
            bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
        } else {
            bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
        }
    }
    return bytes;
}

// 生成 num 位随机整数（对应 DpLogId::GetRandomInt）
function randomIntDigits(num) {
    const min = Math.pow(10, num - 1);
    const max = Math.pow(10, num) - 1;
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

// 对应 detail::DpLogId：生成百度请求所需的 dp-logid
export class DpLogId {
    constructor(client = "") {
        this.client = client;
        this.countId = 0;
        this.sessionId = randomIntDigits(6);
        this.userId = "00" + String(randomIntDigits(8));
    }

    validateUk(uk) {
        return uk.length === 10 && /^[0-9]+$/.test(uk);
    }

    getCountId() {
        if (this.countId < 9999) {
            this.countId++;
        } else {
            this.countId = 0;
        }
        return String(this.countId).padStart(4, "0");
    }

    getDpLogId(uk = "") {
        let innerUserId = this.userId;
        if (uk && this.validateUk(uk)) {
            innerUserId = uk;
        }
        return this.client + String(this.sessionId) + innerUserId + this.getCountId();
    }
}

// 对应 BaiduPcsApi::GenerateRandomFloat：0.1~0.9 之间保留 13 位小数
export function generateRandomFloat() {
    const v = Math.random() * 0.8 + 0.1;
    return v.toFixed(13);
}

// 对应 now_timestamp（秒）
export function nowSeconds() {
    return Math.floor(Date.now() / 1000);
}
