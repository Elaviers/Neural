#pragma once
#include <ELCore/Buffer.hpp>

class LabelsIDX1
{
	Buffer<byte> _labels;

public:
	bool Read(class ByteReader&);
	void Write(class ByteWriter&);

	uint32 GetCount() const { return _labels.GetSize(); }
	byte GetLabel(uint32 index) const { return _labels[index]; }

	void AddLabel(byte label) { _labels.Emplace(label); }
};
