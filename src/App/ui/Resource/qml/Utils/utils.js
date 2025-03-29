
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


// Removes newlines from a string
function removeNewline(str) {
    return str.replace(/(\r\n|\n|\r)/gm, "")
}

// Removes newlines and trims a string
function removeNewlineAndTrim(str) {
    return removeNewline(str).trim()
}

// Returns the parent path of a given path
function getParentPath(path) {
    var p = path
    if (p.startsWith("file:")) {
        p = urlToLocalPath(p)
    }
    return p.substring(0, p.lastIndexOf("/"))
}
