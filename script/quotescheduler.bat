schtasks  /create /TN qutote_watchdog /TR "node c:\wodequant\nodeserver\config\quote_watchdog.js" /SC MINUTE /MO 2 /F