------------------------------------------------------------------------------
-- Separator class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "separator",
  parent = iup.WIDGET,
  creation = "",
  callback = {}
}

function ctrl.createElement(class, arg)
   return iup.Separator()
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
