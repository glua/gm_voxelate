local runtime,exports = ...

--[[
    DOCUMENTATION

    table bf_write:GetTable()
        does nothing?

    bool bf_write:IsValid()
        checks if bf_write is valid

    UCHARPTR bf_write:GetBasePointer()
        gets underlying ucharptr

    number bf_write:GetMaxNumBits()
        returns number of bits capable of being stored in bf_write

    number bf_write:GetNumBitsWritten()
        returns number of bits already written in bf_write

    number bf_write:GetNumBytesWritten()
        returns number of bytes already written in bf_write

    number bf_write:GetNumBitsLeft()
        returns number of bits left to write in bf_write

    number bf_write:GetNumBytesLeft()
        returns number of bytes left to write in bf_write

    boolean bf_write:IsOverflowed()
        returns true if the bf_write is full

    void bf_write:Seek(num bit)
        seeks bf_write to `bit`

    void bf_write:WriteBitAngle(number val,number bits)
        writes some kind of angle number????

    void bf_write:WriteAngle(Angle val)
        writes an angle to the buffer

    boolean bf_write:WriteBits(UCHARPTR val)
        writes a UCHARPTR to the buffer and returns if the operation was successful

    void bf_write:WriteVector(Vector val)
        writes an vector to the buffer

    void bf_write:WriteNormal(Vector val)
        writes an vector normal (?) to the buffer

    void bf_write:WriteByte(number val)
        writes an byte (as a uint8) to the buffer

    boolean bf_write:WriteBytes(UCHARPTR val)
        writes a UCHARPTR to the buffer and returns if the operation was successful

    void bf_write:WriteChar(number val)
        writes an char (as a uint8) to the buffer

    void bf_write:WriteFloat(number val)
        writes an float to the buffer

    void bf_write:WriteDouble(number val)
        writes an double precision float to the buffer

    void bf_write:WriteLong(number val)
        writes an long to the buffer

    void bf_write:WriteLongLong(number val)
        writes an long long to the buffer

    void bf_write:WriteBit(number bit)
        writes a bit to the buffer

    void bf_write:WriteShort(number val)
        writes an short to the buffer

    boolean bf_write:WriteString(string val)
        writes a null terminated string to the buffer and returns if the operation was successful

    void bf_write:WriteInt(number val,number bits)
        writes an int to the buffer

    void bf_write:WriteUInt(number val,number bits)
        writes an unsigned int to the buffer

    void bf_write:WriteWord(number val)
        writes a word (as an int32) to the buffer

    void bf_write:WriteSignedVarInt32(number val)
        no idea what this does internally

    void bf_write:WriteVarInt32(number val)
        no idea what this does internally

    void bf_write:WriteSignedVarInt64(number val)
        no idea what this does internally

    void bf_write:WriteVarInt64(number val)
        no idea what this does internally

    string bf_write:GetString()
        returns the whole UCHARPTR converted to a Lua string

    string bf_write:GetWritenString()
        returns only the written data converted to a Lua string

    table bf_read:GetTable()
        does nothing?

    bool bf_read:IsValid()
        checks if bf_write is valid

    UCHARPTR bf_read:GetBasePointer()
        gets underlying ucharptr

    number bf_read:GetMaxNumBits()
        returns total number of bits in bf_read

    number bf_read:GetNumBitsWritten()
        returns number of bits already read in bf_read

    number bf_read:GetNumBytesWritten()
        returns number of bytes already read in bf_read

    number bf_read:GetNumBitsLeft()
        returns number of bits left to read in bf_read

    number bf_read:GetNumBytesLeft()
        returns number of bytes left to read in bf_read

    boolean bf_read:IsOverflowed()
        returns true if the bf_read has been fully read

    void bf_read:Seek(num bit)
        seeks bf_read to `bit` relative to the start of the buffer

    void bf_read:SeekRelative(num bit)
        seeks bf_read to `bit` relative to current pos

    number bf_read:ReadBitAngle()
        reads a bit angle

    Angle bf_read:ReadAngle()
        reads an angle

    UCHARPTR bf_read:ReadBits(number bits)
        reads specified number of bits and returns as UCHARPTR

    Vector bf_read:ReadVector()
        reads a vector

    number bf_read:ReadNormal()
        reads a vector normal

    number bf_read:ReadByte()
        reads a byte

    (number,bool) bf_read:ReadBytes(number bytes)
        reads specified number of bytes and returns as UCHARPTR and if the operation was a success

    number bf_read:ReadChar()
        reads a char

    number bf_read:ReadFloat()
        reads a float

    number bf_read:ReadDouble()
        reads a double

    number bf_read:ReadLong()
        reads a long

    number bf_read:ReadLongLong()
        reads a long long

    number bf_read:ReadBit()
        reads a bit

    number bf_read:ReadShort()
        reads a short

    (string,boolean) bf_read:ReadString()
        reads a string and returns it as a lua string and if the operation was a success

    number bf_read:ReadInt(number bits)
        reads an int of size `bits`

    number bf_read:ReadUInt(number bits)
        reads an int int of size `bits`

    number bf_read:ReadWord()
        reads a word

    number bf_read:ReadSignedVarInt32()
        no clue what this reads

    number bf_read:ReadVarInt32()
        no clue what this reads

    number bf_read:ReadSignedVarInt64()
        no clue what this reads

    number bf_read:ReadVarInt64()
        no clue what this reads

]]

local UCHARPTR = UCHARPTR
local UCHARPTR_FromString = UCHARPTR_FromString
local sn_bf_read = sn_bf_read
local sn_bf_write = sn_bf_write

local _R = debug.getregistry()

do -- extending bf_write
    function _R.sn_bf_write:WriteData(str,len)
        return self:WriteBytes(UCHARPTR_FromString(str,len),len)
    end

    function _R.sn_bf_write:WriteStringEx(str,len)
        self:WriteData(str,len)
    end
end

function exports.Writer(bytes)
    local gap = 4 - bytes % 4

    local ucptr = UCHARPTR(bytes + gap)

    return sn_bf_write(ucptr)
end

function exports.Reader(data)
    local gap = 4 - #data % 4

    data = data..("\x00"):rep(gap)

    local ucptr = UCHARPTR_FromString(data,#data)

    return sn_bf_read(ucptr)
end
