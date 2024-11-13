
function urlToLocalPath(url) {
    var str = url.toString()
    if (Qt.platform.os === "windows") {
        str = str.replace(/^(file:\/{2,3})/,"")
        if (/^[A-Za-z]:/.test(str)) {
            str = str.replace(/\//g, "\\")
        }
    } else {
        str = str.replace(/^(file:\/{2})/,"")
    }
    return str
}
