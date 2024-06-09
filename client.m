client([0, 0, 0, 0, 0, 0, 0]);

function armState = client(torques)
    persistent client initialized

    if isempty(initialized)
        client = tcpclient('127.0.0.1', 9090, 'Timeout', 5); % Add a timeout
        if isempty(client)
            disp('Failed to connect to server.');
            armState = zeros(1, length(7));
            return;
        else
            initialized = true;
            disp('Client connected to server.');
        end
    end

    % Serialize torques data to send to server
    serializedData = serialize(torques);
    write(client, serializedData, "double");
    
    % Check if there is incoming data and read it
    if client.NumBytesAvailable > 0
        dataReceived = read(client, client.NumBytesAvailable, "double");
        armState = deserialize(dataReceived);
    else
        armState = zeros(1, length(torques)); % Return zeros or last known good value if no data is available
    end

    t = datetime('now', 'Format', 'yyyy-MM-dd HH:mm:ss.SSS');
    disp(['Torques sent at ', char(t), ': ', num2str(torques)]);
    disp(['Arm state received: ', num2str(armState)]);
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