// Demo 插件：验证 JS 插件 SDK 全链路（gdl.http / storage / crypto / utils / log）
// 用法：在应用中粘贴 https://httpbingo.org/anything/demo 触发解析
export default {
    canHandle(url) {
        return url.includes("httpbingo.org");
    },

    async parseUrl(url, userToken) {
        gdl.log.info("parseUrl called: " + url);

        // 验证 http GET + params + headers
        const resp = await gdl.http.get("https://httpbingo.org/anything/list", {
            params: { source: "gdownload-demo" },
            headers: { "X-Demo": "1" }
        });
        gdl.log.info("http status: " + resp.status);
        const echo = resp.json();

        // 验证 storage 读写
        gdl.storage.set("last_parse_url", url);
        gdl.log.debug("stored last_parse_url: " + gdl.storage.get("last_parse_url"));

        // 验证 crypto/utils
        const sig = gdl.crypto.md5(url + (userToken || ""));
        gdl.log.debug("md5 sig: " + sig + ", b64: " + gdl.utils.base64Encode(url));

        // 返回模拟文件列表
        return [
            {
                path: "/",
                name: "demo-1kb.bin",
                size: 1024,
                is_dir: false,
                file_id: "bytes-1024"
            },
            {
                path: "/",
                name: "demo-folder",
                size: 0,
                is_dir: true,
                file_id: "folder-1"
            }
        ];
    },

    async enterDirectory(fileInfo) {
        gdl.log.info("enterDirectory: " + fileInfo.file_id);
        return [
            {
                path: fileInfo.path + fileInfo.name,
                name: "nested-2kb.bin",
                size: 2048,
                is_dir: false,
                file_id: "bytes-2048"
            }
        ];
    },

    async getDownloadInfo(fileInfo) {
        gdl.log.info("getDownloadInfo: " + fileInfo.file_id);
        // httpbin /bytes/N 返回 N 字节随机数据，可真实下载
        const size = fileInfo.file_id.replace("bytes-", "");
        return [
            {
                real_url: "https://httpbingo.org/bytes/" + size,
                file_name: fileInfo.name,
                file_size: fileInfo.size,
                headers: {
                    "User-Agent": "GDownload-Demo/1.0"
                },
                options: {
                    "split": "1",
                    "max-connection-per-server": "1"
                }
            }
        ];
    }
};
