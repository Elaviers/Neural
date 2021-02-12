#pragma once
#include <ELCore/Buffer.hpp>
#include <ELCore/List.hpp>

class ImagesIDX3
{
	Buffer<byte> _imageData;

	uint32 _count;
	uint32 _width;
	uint32 _height;
	uint32 _sz;

	List<Buffer<byte>> _additionalImages;
	uint32 _additionalCount;

public:
	ImagesIDX3() : _count(0), _width(0), _height(0), _additionalCount(0) {}

	bool Read(class ByteReader&);
	void Write(class ByteWriter&);

	uint32 GetCount() const { return _count + _additionalCount; }
	uint32 GetWidth() const { return _width; }
	uint32 GetHeight() const { return _height; }

	const byte* GetImage(uint32 index) const
	{
		if (index < _count)
			return &_imageData[(size_t)index * _sz];

		index -= _count;
		if (index < _additionalCount)
			return _additionalImages.Get(index)->Data();

		return nullptr;
	}

	void AddImage(const Buffer<byte>& image) { _additionalImages.Emplace(image); ++_additionalCount; }
	void AddImage(Buffer<byte>&& image) { _additionalImages.Emplace(std::move(image)); ++_additionalCount; }
};
