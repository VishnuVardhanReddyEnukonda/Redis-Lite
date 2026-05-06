function Start-RedisCli {
    $ip = "127.0.0.1"
    $port = 8080

    try {
        # 1. Connect to the C++ Server
        $client = New-Object System.Net.Sockets.TcpClient($ip, $port)
        $stream = $client.GetStream()
        
        # We use StreamReader and StreamWriter to talk over the raw bytes
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.AutoFlush = $true
        $reader = New-Object System.IO.StreamReader($stream)

        # FIXED SYNTAX HERE:
        Write-Host "Connected to Redis-Lite Server at $($ip):$($port)" -ForegroundColor Cyan
        Write-Host "Type 'exit' or 'quit' to close the client.`n" -ForegroundColor DarkGray

        # 2. The Interactive Loop
        while ($true) {
            $inputCmd = Read-Host -Prompt "redis-lite"
            
            if ($inputCmd -match "^(exit|quit)$") { break }
            if ([string]::IsNullOrWhiteSpace($inputCmd)) { continue }

            # --- RESP ENCODER ---
            $parts = $inputCmd -split '\s+' | Where-Object { $_ -ne '' }
            $resp = "*$($parts.Count)`r`n"
            
            foreach ($p in $parts) {
                $resp += "`$$($p.Length)`r`n$p`r`n"
            }

            # --- SEND & RECEIVE ---
            $writer.Write($resp)
            
            $reply = $reader.ReadLine()
            
            if ($null -eq $reply) {
                Write-Host "Server abruptly closed the connection." -ForegroundColor Red
                break
            }

            # --- RESP DECODER ---
            if ($reply.StartsWith('$')) {
                if ($reply -eq '$-1') {
                    Write-Host "(nil) -> Not Found" -ForegroundColor DarkGray
                } else {
                    $actualData = $reader.ReadLine()
                    Write-Host "`"$actualData`"" -ForegroundColor Green
                }
            } 
            elseif ($reply.StartsWith('+')) {
                Write-Host $reply.Substring(1) -ForegroundColor Green
            } 
            elseif ($reply.StartsWith('-')) {
                Write-Host "(error) $($reply.Substring(1))" -ForegroundColor Red
            } 
            elseif ($reply.StartsWith(':')) {
                Write-Host "(integer) $($reply.Substring(1))" -ForegroundColor Cyan
            } 
            else {
                Write-Host $reply -ForegroundColor Yellow
            }
        }
    } 
    catch {
        # FIXED SYNTAX HERE:
        Write-Host "Could not connect to server at $($ip):$($port). Is your C++ server running?" -ForegroundColor Red
    } 
    finally {
        if ($client) { $client.Close() }
        Write-Host "Disconnected." -ForegroundColor DarkGray
    }
}

# Start the tool!
Start-RedisCli