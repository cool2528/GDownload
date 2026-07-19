// 阿里云盘插件入口(JS 版)
// 走网页端分享 API,下载需在设置里配置 web 端 refresh_token。业务逻辑委托给 lib/ali_api.js
import { AliApi } from "./lib/ali_api.js";

// 单实例:跨 parseUrl → enterDirectory → getDownloadInfo 共享 share_token / access_token / drive_id
const api = new AliApi();

export default {
    canHandle(url) {
        return /alipan\.com|aliyundrive\.com/.test(url);
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
