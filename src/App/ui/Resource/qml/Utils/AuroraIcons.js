.pragma library

const sources = Object.freeze({
    "home": "qrc:/images/icons/aurora/home.svg",
    "download": "qrc:/images/icons/aurora/download.svg",
    "queue": "qrc:/images/icons/aurora/queue.svg",
    "completed": "qrc:/images/icons/aurora/completed.svg",
    "add": "qrc:/images/icons/aurora/add.svg",
    "settings": "qrc:/images/icons/aurora/settings.svg",
    "info": "qrc:/images/icons/aurora/info.svg",
    "help": "qrc:/images/icons/aurora/help.svg",
    "play": "qrc:/images/icons/aurora/play.svg",
    "pause": "qrc:/images/icons/aurora/pause.svg",
    "stop": "qrc:/images/icons/aurora/stop.svg",
    "refresh": "qrc:/images/icons/aurora/refresh.svg",
    "folder": "qrc:/images/icons/aurora/folder.svg",
    "link": "qrc:/images/icons/aurora/link.svg",
    "delete": "qrc:/images/icons/aurora/delete.svg",
    "close": "qrc:/images/icons/aurora/close.svg",
    "cloud": "qrc:/images/icons/aurora/cloud.svg",
    "cloud-download": "qrc:/images/icons/aurora/cloud-download.svg",
    "file": "qrc:/images/icons/aurora/file.svg",
    "warning": "qrc:/images/icons/aurora/warning.svg",
    "error": "qrc:/images/icons/aurora/error.svg",
    "error-badge": "qrc:/images/icons/aurora/error-badge.svg",
    "code": "qrc:/images/icons/aurora/code.svg",
    "connected": "qrc:/images/icons/aurora/connected.svg",
    "shield": "qrc:/images/icons/aurora/shield.svg",
    "keyboard": "qrc:/images/icons/aurora/keyboard.svg",
    "player-settings": "qrc:/images/icons/aurora/player-settings.svg",
    "globe": "qrc:/images/icons/aurora/globe.svg",
    "heart": "qrc:/images/icons/aurora/heart.svg",
    "history": "qrc:/images/icons/aurora/history.svg",
    "open-file": "qrc:/images/icons/aurora/open-file.svg",
    "view": "qrc:/images/icons/aurora/view.svg",
    "extension": "qrc:/images/icons/aurora/extension.svg",
    "lightning": "qrc:/images/icons/aurora/lightning.svg",
    "palette": "qrc:/images/icons/aurora/palette.svg",
    "lock": "qrc:/images/icons/aurora/lock.svg",
    "filter": "qrc:/images/icons/aurora/filter.svg",
    "repository": "qrc:/images/icons/aurora/repository.svg",
    "book": "qrc:/images/icons/aurora/book.svg",
    "people": "qrc:/images/icons/aurora/people.svg",
    "chevron-down": "qrc:/images/icons/aurora/chevron-down.svg",
    "chevron-right": "qrc:/images/icons/aurora/chevron-right.svg",
    "lightbulb": "qrc:/images/icons/aurora/lightbulb.svg",
    "copy": "qrc:/images/icons/aurora/copy.svg",
    "more": "qrc:/images/icons/aurora/more.svg"
});

function has(name) {
    return typeof name === "string" && Object.prototype.hasOwnProperty.call(sources, name);
}

function source(name) {
    if (typeof name !== "string" || !name.trim())
        throw new Error("Aurora icon name must be a non-empty string.");
    const key = name.trim();
    if (!Object.prototype.hasOwnProperty.call(sources, key))
        throw new Error("Unknown Aurora icon: " + name);
    return sources[key];
}
