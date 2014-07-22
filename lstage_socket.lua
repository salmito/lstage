local lstage=require'lstage'
local socket=require'socket'
local mt=getmetatable(socket)
if mt==nil then
	mt={}
	setmetatable(socket,mt)
end

function mt.__persist()
	return function () return require'lstage_socket' end
end

local block=block or false

local tcp_client_mt,err=lstage.getmetatable("tcp{client}")
if tcp_client_mt then
   tcp_client_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1)
      sock:close()
      return function()
         local client_mt,err=lstage.getmetatable("tcp{client}")
         if err then return nil,err end
         local container_sock=assert(socket.tcp())
         container_sock:close()
         container_sock:setfd(sockfd)
         return lstage.setmetatable(container_sock,client_mt)     
      end
   end

   tcp_client_mt.__persist=function(sock)
      error('Unable to send socket "'..tostring(sock)..'" to other processes')
   end


   local old_send=tcp_client_mt.__index.send
   tcp_client_mt.__index.send=function(sock,...)
      lstage.event.waitfd(sock:getfd(),lstage.event.WRITE)
      return old_send(sock,...)
   end
   
      local old_receive=tcp_client_mt.__index.receive
      tcp_client_mt.__index.receive=function(sock,...)
	 lstage.event.waitfd(sock:getfd(),lstage.event.READ)
         return old_receive(sock,...)
      end
   end

local tcp_master_mt,err=lstage.getmetatable("tcp{master}")
if tcp_master_mt then
   tcp_master_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1)
      sock:close()
      return function()
         local client_mt,err=lstage.getmetatable("tcp{master}")
         if err then
            return nil,err
         end
         local container_sock=socket.tcp()
         container_sock:setfd(sockfd)
         return lstage.setmetatable(container_sock,client_mt)     
      end
   end
end

local tcp_server_mt,err=lstage.getmetatable("tcp{server}")
if tcp_server_mt then
   tcp_server_mt.__wrap=function(sock)
      local sockfd=sock:getfd()
      sock:setfd(-1)
      sock:close()
      return function()
         local client_mt,err=lstage.getmetatable("tcp{server}")
         if err then
            return nil,err
         end
         local container_sock=socket.tcp()
         container_sock:setfd(sockfd)
         return lstage.setmetatable(container_sock,client_mt)     
      end
   end
   if not block then   
      local old_accept=tcp_server_mt.__index.accept
      tcp_server_mt.__index.accept=function(sock,...)
	   lstage.event.waitfd(sock:getfd(),lstage.event.READ)
	   return old_accept(sock,...)
      end
   end
end   

return socket

