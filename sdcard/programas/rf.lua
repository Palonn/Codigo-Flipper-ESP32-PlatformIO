-- Radio 2.4 GHz
local function menu(title, opts)
  local sel = 1
  while true do
    os.title(title)
    for i=1,#opts do print((i==sel and "> " or "  ") .. opts[i]) end
    local b = os.joy_wait()
    if b == 1 then sel = sel-1; if sel<1 then sel=#opts end
    elseif b == 2 then sel = sel+1; if sel>#opts then sel=1 end
    elseif b == 3 then return sel
    elseif b == 4 then return nil end
  end
end
while true do
  local s = menu("RF 2.4GHz", {"Escanear","Loopback","Salir"})
  if not s or s == 3 then break end
  if s == 1 then os.rf_scan() else os.rf_loopback() end
end
