set tradename_1=trade_%1%_1
set tradename_2=trade_%1%_2
set tradeapp="%~dp0TradeRunner.exe %2% %3% %4% %5% %6% %7% %8% %9% %10% %11% %12% %13%"
echo %tradeapp_1%
echo %tradeapp_2%
echo %tradename%
schtasks  /create /TN %tradename_1% /TR %tradeapp% /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 08:30  /F
schtasks  /create /TN %tradename_2% /TR %tradeapp% /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 20:30 /F