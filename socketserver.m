server(9090);

function server(port)
    server = tcpserver("0.0.0.0", port);
        
    disp("Server up!")
    global stopServer;
    stopServer = false;

    while ~stopServer
        pause(5);
        t = datetime('now');
        val = sin(2 * pi * (second(t)));
        dataToSend = [0, 0, 0, 0, 0, val, 0];
        % wrong serialization
        serializedData = typecast(dataToSend, 'uint8');

        if server.Connected
            write(server, serializedData)
            % add ms
            disp(['Data sent at ', datestr(t, 'yyyy-mm-dd HH:MM:SS'), ': ', num2str(val)]);
        end
        pause(0.005);
    end

    clear global stopServer;
    delete(server);
end
