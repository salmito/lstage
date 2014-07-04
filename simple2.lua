local l=require'lstage'

local s2=l.stage() --no environment yet

local s=l.stage(function(...)
    print('hello',...)
end)

s2:wrap(function(...) --sets the environment of s2
    s:push(...) --upvalue
end)

s2:push('world')

l.event.sleep(1.0)