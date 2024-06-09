server(9090);

function server(port)
    server = tcpserver("0.0.0.0", port, "ConnectionChangedFcn", @onConnection);
        
    disp("Server up!")
    global stopServer;
    stopServer = false;
    

    desiredAngles = [0, 0, 0, 0, 0, 0, 0];
    armState = [0, 0, 0, 0, 0, 0, 0];

    while ~stopServer
        if server.Connected
            dataReceived = read(server, server.NumBytesAvailable, "double");
            armState = deserialize(dataReceived);

            % Process received data and update desiredAngles as needed
            disp(['Arm State received: ', num2str(armState)]);

            % Example processing, could be more complex based on actual application
            desiredAngles = updateDesiredAngles(armState);

            % Send desired angles back to the client
            serializedData = serialize(desiredAngles);
            write(server, serializedData, "double");

            % Log the transaction
            t = datetime('now');
            disp(['Data sent at ', datestr(t, 'yyyy-mm-dd HH:MM:SS'), ': ', num2str(desiredAngles)]);
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

function desiredAngles = updateDesiredAngles(armState)
    desiredAngles = armState + 1;
end