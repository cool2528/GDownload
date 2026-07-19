// 蓝奏云插件入口(JS 版)
// 分享解析全程无需登录/cookie,业务逻辑委托给 lib/lanzou_api.js
import { LanzouApi } from "./lib/lanzou_api.js";

// 单实例:跨 parseUrl → enterDirectory → getDownloadInfo 共享可用镜像域与提取码
const api = new LanzouApi();

export default {
    canHandle(url) {
        return /lanzou|lanzn/i.test(url);
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
