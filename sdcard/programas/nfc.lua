-- NFC: leer / clonar / emular
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
  local s = menu("NFC", {"Leer tarjeta","Clonar","Emular","Salir"})
  if not s or s == 4 then break end
  if s == 1 then os.nfc_reader()
  elseif s == 2 then os.nfc_clone()
  elseif s == 3 then os.nfc_emulate() end
end
