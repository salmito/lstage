--channels may be used to implement call return pattern
local l=require'lstage'

local chan=l.channel()

local s1=l.stage(function()
    print('very expensive operation')
    for i=1,100000000 do 
      z=(i^2+2*i+2^2)
    end
    chan:push('s1 ended')
end):push()
print('waiting')
print(chan:get())