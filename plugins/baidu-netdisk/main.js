// 百度网盘插件入口（JS 版，替代原生 Baidu_Plugin）
// 对应 baidu_plugin.cxx，业务逻辑委托给 lib/baidu_api.js
import { BaiduApi } from "./lib/baidu_api.js";

// 单实例：跨 parseUrl → enterDirectory → getDownloadInfo 共享 surl/uk/randsk 等状态，
// 与原生版 BaiduPcsApi 生命周期一致
const api = new BaiduApi();

export default {
    canHandle(url) {
        return url.includes("pan.baidu.com");
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
