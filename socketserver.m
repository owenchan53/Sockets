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
        
        serializedData = serialize(dataToSend);

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

function dataBytes = serialize(data)
    % Check endianness
    if ~isequal(typecast(swapbytes(uint16(1)), 'uint8'), [1, 0])
        % The system is little-endian, swap the byte order of the data array
        dataBytes = typecast(swapbytes(data), 'uint8');
    else
        % The system is big-endian, no need to swap bytes
        dataBytes = typecast(data, 'uint8');
    end
end

function data = deserialize(dataBytes)
    dataUint64 = typecast(dataBytes, 'uint64');

    % Check endianness
    if ~isequal(typecast(swapbytes(uint16(1)), 'uint8'), [1, 0])
        % The system is little-endian, swap the byte order
        dataUint64 = swapbytes(dataUint64);
    end

    % Convert the uint64 data back to doubles
    data = typecast(dataUint64, 'double');
end