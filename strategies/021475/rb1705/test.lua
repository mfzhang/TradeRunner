local json = require('rapidjson')
print (0)
manifest=json.decode('[ { "day" : "20170123"},{ "day" : "20170124"}]')

print (1)
print (manifest[1]['day'])