--A stage runs asynchronously
local lstage=require'lstage'

local s=lstage.stage(function()
    print('hello')
end):push()

print(s) --s will be printed first

lstage.event.sleep(1.0) --necessary for s to run