local runtime,exports = ...

local IO = runtime.oop.create("I/O")
exports.IO = IO

function IO:__ctor(tag,tagColor,msgColor)
    self.tag = tag
    self.tagColor = tagColor
    self.msgColor = msgColor

    self.debug = false
end

function IO:SetDebug(debug)
    self.debug = debug
end

function IO:NewLine()
    Msg("\n")
end

function IO:Tag(tagName)
    MsgC(self.tagColor,string.format("[%s] ",tagName or self.tag))
end

function IO:Print(fmt,...)
    self:Tag()
    self:Tag("OUT")
    MsgC(self.msgColor,string.format(fmt,...))
    self:NewLine()
end

function IO:PrintDebug(fmt,...)
    if not self.debug then return end

    self:Tag()
    self:Tag("DBG")
    MsgC(self.msgColor,string.format(fmt,...))
    self:NewLine()
end

function IO:PrintError(fmt,...)
    self:Tag()
    self:Tag("ERR")
    MsgC(self.msgColor,string.format(fmt,...))
    self:NewLine()
end
