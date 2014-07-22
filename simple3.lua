--Stage communicate through asynchronous channels
--(no globals)
local l=require'lstage'

local s2=l.stage() --no environment yet

local chan=l.channel() --Asynchronous channel

local s=l.stage(function()
    print('hello',chan:get()) --stage sleeps if channel is empty
end):push()

s2:wrap(function(...)
    chan:push(...) 
end)

s2:push('world')

l.event.sleep(1.0)