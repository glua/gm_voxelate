#include <sn_bf_read.hpp>
#include <sn_ucharptr.hpp>
#include <bitbuf.h>

namespace sn_bf_read
{

struct Container
{
	bf_read *preader;
	bf_read reader;
	int32_t bufref;
};

static uint8_t metatype = GarrysMod::Lua::Type::NONE;
static const char *metaname = "sn_bf_read";

bf_read **Push( GarrysMod::Lua::ILuaBase *LAU, bf_read *reader, int32_t bufref )
{
	Container *container = LAU->NewUserType<Container>( metatype );
	container->bufref = bufref;

	if( reader == nullptr )
		container->preader = new( &container->reader ) bf_read;
	else
		container->preader = reader;

	LAU->PushMetaTable( metatype );
	LAU->SetMetaTable( -2 );

	LAU->CreateTable( );
	lua_setfenv( LAU->GetLuaState(), -2 );

	return &container->preader;
}

inline Container *GetUserData( GarrysMod::Lua::ILuaBase *LAU, int32_t index )
{
	sn_bf_common::CheckType( LAU, index, metatype, metaname );
	return LAU->GetUserType<Container>( index, metatype );
}

bf_read *Get( GarrysMod::Lua::ILuaBase *LAU, int32_t index, int32_t *bufref )
{
	Container *container = GetUserData( LAU, index );
	bf_read *reader = container->preader;
	if( reader == nullptr )
		sn_bf_common::ThrowError( LAU, "invalid %s", metaname );

	if( bufref != nullptr )
		*bufref = container->bufref;

	return reader;
}

LUA_FUNCTION_STATIC( gc )
{
	Container *container = GetUserData( LUA, 1 );

	LUA->ReferenceFree( container->bufref );

	LUA->SetUserType( 1, nullptr );

	return 0;
}

LUA_FUNCTION_STATIC( eq )
{
	bf_read *buf1 = Get( LUA, 1 );
	bf_read *buf2 = Get( LUA, 2 );

	LUA->PushBool( buf1 == buf2 );

	return 1;
}

LUA_FUNCTION_STATIC( tostring )
{
	bf_read *buf = Get( LUA, 1 );

	lua_pushfstring( LUA->GetLuaState(), sn_bf_common::tostring_format, metaname, buf );

	return 1;
}

LUA_FUNCTION_STATIC( IsValid )
{
	sn_bf_common::CheckType( LUA, 1, metatype, metaname );

	LUA->PushBool( GetUserData( LUA, 1 )->preader != nullptr );

	return 1;
}

LUA_FUNCTION_STATIC( GetBasePointer )
{
	int32_t bufref = LUA_NOREF;
	bf_read *buf = Get( LUA, 1, &bufref );

	if( bufref != LUA_NOREF )
		LUA->ReferencePush( bufref );
	else
		UCHARPTR::Push( LUA, buf->m_nDataBits, const_cast<uint8_t *>( buf->GetBasePointer( ) ) );

	return 1;
}

LUA_FUNCTION_STATIC( TotalBytesAvailable )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->TotalBytesAvailable( ) );

	return 1;
}

LUA_FUNCTION_STATIC( GetNumBitsLeft )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->GetNumBitsLeft( ) );

	return 1;
}

LUA_FUNCTION_STATIC( GetNumBytesLeft )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->GetNumBytesLeft( ) );

	return 1;
}

LUA_FUNCTION_STATIC( GetNumBitsRead )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->GetNumBitsRead( ) );

	return 1;
}

LUA_FUNCTION_STATIC( GetNumBytesRead )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->GetNumBytesRead( ) );

	return 1;
}

LUA_FUNCTION_STATIC( IsOverflowed )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushBool( buf->IsOverflowed( ) );

	return 1;
}

LUA_FUNCTION_STATIC( Seek )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	LUA->PushBool( buf->Seek( static_cast<int32_t>( LUA->GetNumber( 2 ) ) ) );

	return 1;
}

LUA_FUNCTION_STATIC( SeekRelative )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	LUA->PushBool( buf->SeekRelative( static_cast<int32_t>( LUA->GetNumber( 2 ) ) ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadBitAngle )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t bits = static_cast<int32_t>( LUA->GetNumber( 2 ) );
	if( bits < 0 )
		sn_bf_common::ThrowError( LUA, "invalid number of bits to read (%d is less than 0)", bits );

	LUA->PushNumber( buf->ReadBitAngle( bits ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadAngle )
{
	bf_read *buf = Get( LUA, 1 );

	QAngle ang;
	buf->ReadBitAngles( ang );

	LUA->PushAngle( ang );

	return 1;
}

LUA_FUNCTION_STATIC( ReadBits )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t bits = static_cast<int32_t>( LUA->GetNumber( 2 ) );

	uint8_t *data = UCHARPTR::Push( LUA, bits );

	buf->ReadBits( data, bits );

	return 1;
}

LUA_FUNCTION_STATIC( ReadVector )
{
	bf_read *buf = Get( LUA, 1 );

	Vector vec;
	buf->ReadBitVec3Coord( vec );

	LUA->PushVector( vec );

	return 1;
}

LUA_FUNCTION_STATIC( ReadNormal )
{
	bf_read *buf = Get( LUA, 1 );

	Vector vec;
	buf->ReadBitVec3Normal( vec );

	LUA->PushVector( vec );

	return 1;
}

LUA_FUNCTION_STATIC( ReadByte )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadByte( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadBytes )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t bytes = static_cast<int32_t>( LUA->GetNumber( 2 ) );

	uint8_t *data = UCHARPTR::Push( LUA, bytes * 8 );

	LUA->PushBool( buf->ReadBytes( data, bytes ) );

	return 2;
}

LUA_FUNCTION_STATIC( ReadChar )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadChar( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadFloat )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadFloat( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadDouble )
{
	bf_read *buf = Get( LUA, 1 );

	double num = 0.0;
	buf->ReadBits( &num, sizeof( double ) * 8 );
	LUA->PushNumber( num );

	return 1;
}

LUA_FUNCTION_STATIC( ReadLong )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadLong( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadLongLong )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadLongLong( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadBit )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadOneBit( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadShort )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadShort( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadString )
{
	bf_read *buf = Get( LUA, 1 );

	char str[2048] = { 0 };
	bool success = buf->ReadString( str, sizeof( str ) );

	LUA->PushString( str );
	LUA->PushBool( success );

	return 2;
}

LUA_FUNCTION_STATIC( ReadInt )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t bits = static_cast<int32_t>( LUA->GetNumber( 2 ) );
	if( bits < 0 || bits > 32 )
		sn_bf_common::ThrowError(
			LUA,
			"invalid number of bits to read (%d is not between 0 and 32)",
			bits
		);

	LUA->PushNumber( buf->ReadSBitLong( bits ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadUInt )
{
	bf_read *buf = Get( LUA, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::NUMBER );

	int32_t bits = static_cast<int32_t>( LUA->GetNumber( 2 ) );
	if( bits < 0 || bits > 32 )
		sn_bf_common::ThrowError(
			LUA,
			"invalid number of bits to read (%d is not between 0 and 32)",
			bits
		);

	LUA->PushNumber( buf->ReadUBitLong( bits ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadWord )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadWord( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadSignedVarInt32 )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadSignedVarInt32( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadVarInt32 )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadVarInt32( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadSignedVarInt64 )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadSignedVarInt64( ) );

	return 1;
}

LUA_FUNCTION_STATIC( ReadVarInt64 )
{
	bf_read *buf = Get( LUA, 1 );

	LUA->PushNumber( buf->ReadVarInt64( ) );

	return 1;
}

LUA_FUNCTION_STATIC( Constructor )
{
	int32_t bits = 0;
	uint8_t *ptr = UCHARPTR::Get( LUA, 1, &bits );

	LUA->Push( 1 );
	bf_read *reader = *Push( LUA, nullptr, LUA->ReferenceCreate( ) );
	reader->StartReading( ptr, BitByte( bits ), 0, bits );

	return 1;
}

void Initialize( GarrysMod::Lua::ILuaBase *LAU )
{
	metatype = LAU->CreateMetaTable( metaname );

		LAU->PushCFunction( gc );
		LAU->SetField( -2, "__gc" );

		LAU->PushCFunction( eq );
		LAU->SetField( -2, "__eq" );

		LAU->PushCFunction( tostring );
		LAU->SetField( -2, "__tostring" );

		LAU->PushCFunction( sn_bf_common::index);
		LAU->SetField( -2, "__index" );

		LAU->PushCFunction( sn_bf_common::newindex );
		LAU->SetField( -2, "__newindex" );

		LAU->PushCFunction( sn_bf_common::GetTable );
		LAU->SetField( -2, "GetTable" );

		LAU->PushCFunction( IsValid );
		LAU->SetField( -2, "IsValid" );

		LAU->PushCFunction( GetBasePointer );
		LAU->SetField( -2, "GetBasePointer" );

		LAU->PushCFunction( TotalBytesAvailable );
		LAU->SetField( -2, "TotalBytesAvailable" );

		LAU->PushCFunction( GetNumBitsLeft );
		LAU->SetField( -2, "GetNumBitsLeft" );

		LAU->PushCFunction( GetNumBytesLeft );
		LAU->SetField( -2, "GetNumBytesLeft" );

		LAU->PushCFunction( GetNumBitsRead );
		LAU->SetField( -2, "GetNumBitsRead" );

		LAU->PushCFunction( GetNumBytesRead );
		LAU->SetField( -2, "GetNumBytesRead" );

		LAU->PushCFunction( IsOverflowed );
		LAU->SetField( -2, "IsOverflowed" );

		LAU->PushCFunction( Seek );
		LAU->SetField( -2, "Seek" );

		LAU->PushCFunction( SeekRelative );
		LAU->SetField( -2, "SeekRelative" );

		LAU->PushCFunction( ReadBitAngle );
		LAU->SetField( -2, "ReadBitAngle" );

		LAU->PushCFunction( ReadAngle );
		LAU->SetField( -2, "ReadAngle" );

		LAU->PushCFunction( ReadBits );
		LAU->SetField( -2, "ReadBits" );

		LAU->PushCFunction( ReadVector );
		LAU->SetField( -2, "ReadVector" );

		LAU->PushCFunction( ReadNormal );
		LAU->SetField( -2, "ReadNormal" );

		LAU->PushCFunction( ReadByte );
		LAU->SetField( -2, "ReadByte" );

		LAU->PushCFunction( ReadBytes );
		LAU->SetField( -2, "ReadBytes" );

		LAU->PushCFunction( ReadChar );
		LAU->SetField( -2, "ReadChar" );

		LAU->PushCFunction( ReadFloat );
		LAU->SetField( -2, "ReadFloat" );

		LAU->PushCFunction( ReadDouble );
		LAU->SetField( -2, "ReadDouble" );

		LAU->PushCFunction( ReadLong );
		LAU->SetField( -2, "ReadLong" );

		LAU->PushCFunction( ReadLongLong );
		LAU->SetField( -2, "ReadLongLong" );

		LAU->PushCFunction( ReadBit );
		LAU->SetField( -2, "ReadBit" );

		LAU->PushCFunction( ReadShort );
		LAU->SetField( -2, "ReadShort" );

		LAU->PushCFunction( ReadString );
		LAU->SetField( -2, "ReadString" );

		LAU->PushCFunction( ReadInt );
		LAU->SetField( -2, "ReadInt" );

		LAU->PushCFunction( ReadUInt );
		LAU->SetField( -2, "ReadUInt" );

		LAU->PushCFunction( ReadWord );
		LAU->SetField( -2, "ReadWord" );

		LAU->PushCFunction( ReadSignedVarInt32 );
		LAU->SetField( -2, "ReadSignedVarInt32" );

		LAU->PushCFunction( ReadVarInt32 );
		LAU->SetField( -2, "ReadVarInt32" );

		LAU->PushCFunction( ReadSignedVarInt64 );
		LAU->SetField( -2, "ReadSignedVarInt64" );

		LAU->PushCFunction( ReadVarInt64 );
		LAU->SetField( -2, "ReadVarInt64" );

	LAU->Pop( 1 );

	LAU->PushCFunction( Constructor );
	LAU->SetField( GarrysMod::Lua::INDEX_GLOBAL, metaname );
}

void Deinitialize( GarrysMod::Lua::ILuaBase *LAU )
{
	LAU->PushNil( );
	LAU->SetField( GarrysMod::Lua::INDEX_GLOBAL, metaname );

	LAU->PushNil( );
	LAU->SetField( GarrysMod::Lua::INDEX_REGISTRY, metaname );
}

}
