set binpath=c:\wodequant\traderunner\
set investor=%1%
set tradename_1=trade_%investor%_1
set tradename_2=trade_%investor%_2

rem 忽略第一个参数

echo %tradeapp_1%
echo %tradeapp_2%
set startupapp=%binpath%starttrade_%investor%.bat

schtasks  /create /TN %tradename_1% /TR "%startupapp%" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 08:30  /F
schtasks  /create /TN %tradename_2% /TR "%startupapp%" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 20:30  /F