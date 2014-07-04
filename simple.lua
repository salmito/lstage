local l=require'lstage'

local s=l.stage(function()
    print('hello')
end):push()

print(s)

l.event.sleep(1.0)