
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

// Splits string paths by newlines Returns a list of string paths
function splitPath(path) {
    return path.split(/\r\n|\r|\n/)
}
