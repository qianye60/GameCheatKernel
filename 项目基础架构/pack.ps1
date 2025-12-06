$d = Split-Path -Parent $MyInvocation.MyCommand.Path
$in = Join-Path $d "libs\arm64-v8a\QianyeKernel"
$out = Join-Path $d "QianyeKernel.sh"

if (!(Test-Path $in)) {
    Write-Host "[-] Not found: $in" -ForegroundColor Red
    exit 1
}

$h = @"
#!/bin/bash
TEMP_DIR=/data/local/tmp
TEMP_FILE=`$TEMP_DIR/qk_`$(date +%s)
tail -n +10 "`$0" | base64 -d > "`$TEMP_FILE"
chmod 755 "`$TEMP_FILE"
"`$TEMP_FILE" "`$@"
RET=`$?
rm -f "`$TEMP_FILE"
exit `$RET
"@

$b = [Convert]::ToBase64String([IO.File]::ReadAllBytes($in))
$lines = @()
for ($i = 0; $i -lt $b.Length; $i += 76) {
    $lines += $b.Substring($i, [Math]::Min(76, $b.Length - $i))
}
$c = $h + "`n" + ($lines -join "`n")
[IO.File]::WriteAllText($out, $c.Replace("`r`n", "`n"), (New-Object Text.UTF8Encoding $false))
Write-Host "[+] Done: $out" -ForegroundColor Green
