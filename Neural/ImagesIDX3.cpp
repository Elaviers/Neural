#include "ImagesIDX3.hpp"
#include <ELCore/ByteReader.hpp>
#include <ELCore/ByteWriter.hpp>
#include <ELSys/Debug.hpp>

bool ImagesIDX3::Read(ByteReader& reader)
{
	if (reader.Read_uint32() != 2051)
	{
		Debug::Error("IDX3 magic number incorrect!");
		return false;
	}
	
	_count = reader.Read_uint32();
	_height = reader.Read_uint32();
	_width = reader.Read_uint32();
	_sz = _width * _height;

	_imageData.SetSize((size_t)_count * _width * _height);
	if (reader.Read(_imageData.Data(), _imageData.GetSize()) != _imageData.GetSize())
	{
		Debug::Error("Bad IDX3 file");
		return false;
	}

	return true;
}

void ImagesIDX3::Write(ByteWriter& writer)
{
	writer.EnsureSpace((size_t)4 * 4 + (_count + _additionalCount) * _sz);
	writer.Write_uint32(2051);
	writer.Write_uint32(_count + _additionalCount);
	writer.Write_uint32(_height);
	writer.Write_uint32(_width);

	writer.Write(_imageData.Data(), _imageData.GetSize());

	for (Buffer<byte>& data : _additionalImages)
		writer.Write(data.Data(), data.GetSize());
}
