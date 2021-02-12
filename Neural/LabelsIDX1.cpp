#include "LabelsIDX1.hpp"
#include <ELCore/ByteReader.hpp>
#include <ELCore/ByteWriter.hpp>
#include <ELSys/Debug.hpp>

bool LabelsIDX1::Read(ByteReader& reader)
{
	if (reader.Read_uint32() != 2049)
	{
		Debug::Error("IDX1 magic number incorrect!");
		return false;
	}

	_labels.SetSize(reader.Read_uint32());
	if (reader.Read(_labels.Data(), _labels.GetSize()) != _labels.GetSize())
	{
		Debug::Error("Bad IDX1 file");
		return false;
	}

	return true;
}

void LabelsIDX1::Write(ByteWriter& writer)
{
	writer.EnsureSpace(2 * 4 + _labels.GetSize());
	writer.Write_uint32(2049);
	writer.Write_uint32(_labels.GetSize());
	writer.Write(_labels.Data(), _labels.GetSize());
}
