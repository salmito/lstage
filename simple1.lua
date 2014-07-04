local l=require'lstage'

local s=l.stage(function()
    print('hello')
    
    local s2=l.stage(function()
        print'world' 
        print(l.self(),l.self():parent())
    end):push()    
  
end):push()

print(s)

l.event.sleep(1.0)