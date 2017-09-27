echo Part 1/4: Make
@..\..\..\tools\make.exe %*
echo Part 2/4: Finalize
CALL "..\..\..\toolchain\win_pnacl\bin\pnacl-finalize.bat" "pnacl\Release\pnacl_player.pexe" -o "pnacl\Release\pnacl_player.final.pexe"
echo Part 3/4: Compress
CALL "..\..\..\toolchain\win_pnacl\bin\pnacl-compress.bat" "pnacl\Release\pnacl_player.final.pexe"
echo Part 4/4: Copy Output
copy "pnacl\Release\pnacl_player.final.pexe" "FinishedOutput\pnacl_player.pexe"
copy "pnacl\Release\pnacl_player.nmf" "FinishedOutput\pnacl_player.nmf"
echo Finalized, compressed output is pnacl_player.pexe and pnacl_player.nmf in "FinishedOutput\"
copy "pnacl\Release\pnacl_player.pexe" "V:\Blue Iris 4\www\pnacl_player\pnacl\Release"