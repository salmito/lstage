local l=require'lstage'

--stages are first class values...
local upvalue=l.stage(function (s)
    print"I'm an upvalue!"
    s:push()
end)

local s=l.stage(function()
    print('hello')
    -- ... and can be nested    
    local s2=l.stage(function()
        print'world' 
        print(l.self(),l.self():parent())
    end)  
    upvalue:push(s2) --and passed around
  
end):push()

print(s)

l.event.sleep(1.0)