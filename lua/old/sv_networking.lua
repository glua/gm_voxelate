local IMPORTS = gm_voxelate.IMPORTS
local CONFIGS = gm_voxelate.CONFIGS

util.AddNetworkString("voxelate_req")

util.AddNetworkString("voxelate_init_start")
util.AddNetworkString("voxelate_init_chunks")
util.AddNetworkString("voxelate_init_finish")
util.AddNetworkString("voxelate_init_restart")

util.AddNetworkString("voxelate_explicit_delete")

util.AddNetworkString("voxelate_set")
util.AddNetworkString("voxelate_set_region")
util.AddNetworkString("voxelate_set_sphere")

local INIT_TASKS = {}
local COMPLETED_TASKS = {}

hook.Add("PlayerInitialSpawn","voxelate_ply_join",function(ply)
	INIT_TASKS[ply]={}
	COMPLETED_TASKS[ply] = {}
end)

hook.Add("PlayerDisconnected","voxelate_ply_leave",function(ply)
	INIT_TASKS[ply]=nil
	COMPLETED_TASKS[ply]=nil
end)

hook.Add("Tick","voxelate_stream_tick",function()
	for ply,tasks in pairs(INIT_TASKS) do
		local to_delete = {}
		for index,chunks_left in pairs(tasks) do
			local size_limit = 2000
			local size_written = 0
			net.Start("voxelate_init_chunks")
			net.WriteUInt(index,16)
			while size_written<size_limit do
				local cpos = table.remove(chunks_left)

				local data = IMPORTS.voxData(index,cpos.x,cpos.y,cpos.z) -- THIS IS VERY BROKEN
				if data == nil then to_delete[index]=false break end

				net.WriteUInt(cpos.x,16)
				net.WriteUInt(cpos.y,16)
				net.WriteUInt(cpos.z,16)
				net.WriteUInt(#data,16)
				net.WriteData(data,#data)

				size_written = size_written + #data
				if #chunks_left == 0 then
					to_delete[index]=true
					break
				end
			end
			net.WriteUInt(65535,16)
			net.Send(ply)
		end
		for index,success in pairs(to_delete) do
			tasks[index]=nil

			if success then
				net.Start("voxelate_init_finish")

				net.WriteUInt(index,16)
				net.Send(ply)

				COMPLETED_TASKS[ply][index]=true
			end
		end
	end
end)

net.Receive("voxelate_req",function(len,ply)
	local index = net.ReadUInt(16)

	if CONFIGS[index] then

		net.Start("voxelate_init_start")
		net.WriteUInt(index,16)
		net.WriteTable(CONFIGS[index])
		net.Send(ply)

		INIT_TASKS[ply][index]=IMPORTS.voxGetAllChunks(index)
	end
end)

function IMPORTS.voxReInit(index)
	print("CALLED RE-INIT, FIXME")

	local plys = {}
	for ply,tasks in pairs(INIT_TASKS) do
		if tasks[index] then tasks[index]= IMPORTS.voxGetAllChunks(index) table.insert(plys,ply) end
	end
	for ply,tasks in pairs(COMPLETED_TASKS) do
		if tasks[index] then tasks[index]=nil INIT_TASKS[ply][index]= IMPORTS.voxGetAllChunks(index) table.insert(plys,ply) end
	end

	net.Start("voxelate_init_restart")
	net.WriteUInt(index,16)
	net.Send(plys)
end
