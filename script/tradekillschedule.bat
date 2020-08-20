schtasks  /create /TN tradekill_1 /TR "TASKKILL /F /IM TradeRunner.exe /T" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 16:00  /F
schtasks  /create /TN strategykill_1 /TR "TASKKILL /F /IM TradeStrategy.exe /T" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 16:00  /F
schtasks  /create /TN  tradekill_2 /TR "TASKKILL /F /IM TradeRunner.exe /T" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 03:00 /F
schtasks  /create /TN  strategykill_2 /TR "TASKKILL /F /IM TradeStrategy.exe /T" /SC WEEKLY  /D MON,TUE,WED,THU,FRI /ST 03:00 /F
