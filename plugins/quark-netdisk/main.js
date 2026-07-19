// 夸克网盘插件入口(JS 版)
// 业务逻辑委托给 lib/quark_api.js
import { QuarkApi } from "./lib/quark_api.js";

// 单实例:跨 parseUrl → enterDirectory → getDownloadInfo 共享 stoken / 逐文件 token 等状态
const api = new QuarkApi();

export default {
    canHandle(url) {
        return url.includes("pan.quark.cn");
    },

    async parseUrl(url, userToken) {
        return api.parseUrl(url, userToken);
    },

    async enterDirectory(fileInfo) {
        return api.enterDirectory(fileInfo);
    },

    async getDownloadInfo(fileInfo) {
        return api.getDownloadInfo(fileInfo);
    }
};
