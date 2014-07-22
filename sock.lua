local socket=require'lstage_socket'
local lstage=require'lstage'
local sock=socket.udp()
local sock2=socket.udp()

local udp_port=6666
sock:setsockname('*', udp_port)


local s1=lstage.stage(function(cli)
	print('handling client',cli)	
	cli:send("Welcome stranger\r\n")
	local str=cli:receive()
	print('received',str)
	cli:send('ECHO: '..str..'\r\n')
	print('closing client',cli)
	cli:close()
end)



local s2=lstage.stage(function(port)	
	print(sock:receivefrom())
	local server_sock=assert(socket.bind("*",port))
       print("SERVER: Waiting on port >> ",port,server_sock)
       while true do
          local cli_sock=server_sock:accept()
          print("SERVER: Sending client")
          print('Client socket',cli_sock)
     	s1:push(cli_sock)
       end
end):push(9999)

local s3=lstage.stage(function()
	sock2:sendto("hello world",'127.0.0.1',udp_port)
end):push()


lstage.channel():get()