
local set = internals.voxSet
local fix = math.floor

internals.voxSetRegion = function(index,x,y,z,sx,sy,sz,d)

	x = fix(x)
	y = fix(y)
	z = fix(z)
	sx = fix(sx)
	sy = fix(sy)
	sz = fix(sz)

	local success = false

	for ix=x,x+sx do
		for iy=y,y+sy do
			for iz=z,z+sz do
				success = set(index,ix,iy,iz,d) or success
			end
		end
	end

	return success
end

internals.voxSetSphere = function(index,x,y,z,r,d)
	
	x = fix(x)
	y = fix(y)
	z = fix(z)
	r = fix(r)
	
	local success = false

	local rsqr = r*r
	for ix=x-r,x+r do
		local xsqr = (ix-x)*(ix-x)
		for iy=y-r,y+r do
			local xysqr = xsqr+(iy-y)*(iy-y)
			for iz=z-r,z+r do
				local xyzsqr = xysqr+(iz-z)*(iz-z)
				if xyzsqr<=rsqr then
					success = set(index,ix,iy,iz,d) or success
				end
			end
		end
	end

	return success
end