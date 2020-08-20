set tradename=trade_%1%_1
echo %tradename%
schtasks  /delete /TN "%tradename%" /F
set tradename=trade_%1%_2
echo %tradename%
schtasks  /delete /TN "%tradename%" /F