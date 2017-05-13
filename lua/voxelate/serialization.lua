local runtime,exports = ...

local bON = dofile("deps/bon.lua")

function exports.serialize(data)
	return bON.serialize(data)
end

function exports.deserialize(data)
	return bON.deserialize(data)
end

-- function exports.serialize(data)
--	 return util.TableToJSON(data)
-- end

-- function exports.deserialize(data)
--	 return util.JSONToTable(data)
-- end
