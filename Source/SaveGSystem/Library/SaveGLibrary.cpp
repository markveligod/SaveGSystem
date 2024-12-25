// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGLibrary.h"
#include "Compression/CompressedBuffer.h"


void USaveGLibrary::CompressData(const TArray<uint8>& SomeData, TArray<uint8>& OutData)
{
    // Create an FSharedBuffer from the uncompressed data
    const FSharedBuffer UncompressedBuffer = FSharedBuffer::MakeView(SomeData.GetData(), SomeData.Num());

    // Compress the data with the desired compression level
    const FCompressedBuffer CompressedBuffer = FCompressedBuffer::Compress(
        UncompressedBuffer,
        ECompressedBufferCompressor::Kraken,
        ECompressedBufferCompressionLevel::Normal
    );

    // Retrieve the compressed data
    const FSharedBuffer SharedCompressedData = CompressedBuffer.GetCompressed().ToShared();
    const void* DataPtr = SharedCompressedData.GetData();
    const int64 DataSize = SharedCompressedData.GetSize();

    // Copy the compressed data to the output array
    OutData.SetNumUninitialized(DataSize);
    FMemory::Memcpy(OutData.GetData(), DataPtr, DataSize);
}

bool USaveGLibrary::DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData)
{
    // Create an FSharedBuffer from the compressed data
    const FSharedBuffer SharedCompressedData = FSharedBuffer::MakeView(CompressedData.GetData(), CompressedData.Num());

    // Create a compressed buffer from the shared buffer
    const FCompressedBuffer CompressedBuffer = FCompressedBuffer::FromCompressed(SharedCompressedData);

    // Decompress the data
    const FSharedBuffer DecompressedBuffer = CompressedBuffer.Decompress();

    // Retrieve the decompressed data
    const void* DataPtr = DecompressedBuffer.GetData();
    const int64 DataSize = DecompressedBuffer.GetSize();

    // Copy the decompressed data to the output array
    OutData.SetNumUninitialized(DataSize);
    FMemory::Memcpy(OutData.GetData(), DataPtr, DataSize);

    return true;
}