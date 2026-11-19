$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root "assets"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function New-RoundedPath([double]$x, [double]$y, [double]$w, [double]$h, [double]$r) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-IconBitmap([int]$s) {
    $bmp = New-Object System.Drawing.Bitmap($s, $s, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $bg = New-RoundedPath 0 0 $s $s ([Math]::Max(2.0, $s * 0.24))
    $rect = New-Object System.Drawing.Rectangle(0, 0, $s, $s)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $rect,
        [System.Drawing.Color]::FromArgb(255, 143, 113, 255),
        [System.Drawing.Color]::FromArgb(255, 79, 48, 214),
        45.0)
    $g.FillPath($brush, $bg)

    $white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    $body = New-RoundedPath ($s * 0.22) ($s * 0.17) ($s * 0.56) ($s * 0.71) ($s * 0.08)
    $g.FillPath($white, $body)
    $clip = New-RoundedPath ($s * 0.36) ($s * 0.09) ($s * 0.28) ($s * 0.16) ($s * 0.05)
    $g.FillPath($white, $clip)

    if ($s -ge 24) {
        $ink = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 84, 54, 224))
        $lineH = [Math]::Max(1.5, $s * 0.05)
        $x = $s * 0.33
        $w1 = $s * 0.34
        foreach ($y in @(0.40, 0.52, 0.64)) {
            $p = New-RoundedPath $x ($s * $y) $w1 $lineH ([Math]::Max(1.0, $lineH / 2))
            $g.FillPath($ink, $p)
        }
        $ink.Dispose()
    }

    $clip.Dispose()
    $body.Dispose()
    $white.Dispose()
    $brush.Dispose()
    $bg.Dispose()
    $g.Dispose()
    return $bmp
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$pngs = @()
foreach ($s in $sizes) {
    $bmp = New-IconBitmap $s
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += [PSCustomObject]@{ Size = $s; Bytes = $ms.ToArray() }
    if ($s -eq 256) {
        $bmp.Save((Join-Path $outDir "cloppy.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    }
    $ms.Dispose()
    $bmp.Dispose()
}

$icoPath = Join-Path $outDir "cloppy.ico"
$fs = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([UInt16]0)
$bw.Write([UInt16]1)
$bw.Write([UInt16]$pngs.Count)
$offset = 6 + 16 * $pngs.Count
foreach ($entry in $pngs) {
    $dim = if ($entry.Size -ge 256) { 0 } else { $entry.Size }
    $bw.Write([byte]$dim)
    $bw.Write([byte]$dim)
    $bw.Write([byte]0)
    $bw.Write([byte]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]$entry.Bytes.Length)
    $bw.Write([UInt32]$offset)
    $offset += $entry.Bytes.Length
}
foreach ($entry in $pngs) {
    $bw.Write($entry.Bytes)
}
$bw.Flush()
$bw.Dispose()
$fs.Dispose()
Write-Host "Wrote $icoPath"
