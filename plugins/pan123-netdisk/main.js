// 123云盘插件入口(JS 版)
// 列目录匿名可用;下载需在设置里配置登录 token。业务逻辑委托给 lib/pan123_api.js
import { Pan123Api } from "./lib/pan123_api.js";

// 单实例:跨 parseUrl → enterDirectory → getDownloadInfo 共享 shareKey / token / 文件元数据
const api = new Pan123Api();

export default {
    canHandle(url) {
        return /123pan|123684|123865|123912|123952|123624|123254|123957|123295|123860|123245|123278|123842|123294|123773|123641|123259|123652|123635|123242|123795/.test(url);
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
