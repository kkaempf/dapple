byte ReadRawDiskIO( word Address );
void WriteRawDiskIO( word Address, byte Data );

void InitMassStor( int slot );
void ShutdownMassStor( void );
byte ReadMassStorIO( word Address );
void WriteMassStorIO( word Address, byte Data );
