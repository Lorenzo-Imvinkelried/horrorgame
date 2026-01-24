$deps = @{
    "sfml"          = "https://github.com/SFML/SFML/archive/refs/tags/2.6.1.zip"
    "glm"           = "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip"
    "glad"          = "https://github.com/libigl/libigl-glad/archive/refs/heads/master.zip"
    "tinyobjloader" = "https://github.com/tinyobjloader/tinyobjloader/archive/refs/heads/master.zip"
    "stb"           = "https://github.com/nothings/stb/archive/master.zip"
}

$libDir = "libs"
if (!(Test-Path $libDir)) { New-Item -ItemType Directory -Path $libDir | Out-Null }

foreach ($name in $deps.Keys) {
    $url = $deps[$name]
    $output = Join-Path $libDir "$name.zip"
    
    # Skip if already exists (simple cache)
    if (Test-Path $output) {
        Write-Host "  -> Skipping $name (already exists)"
        continue
    }

    Write-Host "Downloading $name from $url..."
    try {
        Start-BitsTransfer -Source $url -Destination $output -ErrorAction Stop
        Write-Host "  -> Success: $output"
    }
    catch {
        Write-Host "  -> Failed BITS, trying WebRequest..."
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing
            Write-Host "  -> Success (WebRequest): $output"
        }
        catch {
            Write-Error "  -> Failed to download $name : $_"
        }
    }
}
Write-Host "Dependency download process completed."
