// Copyright (C) 2023-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "openvino/genai/llm_pipeline.hpp"
#include <fstream>

std::pair<std::string, ov::Tensor> decrypt_model(const std::string& model_path, const std::string& weights_path) {
    std::ifstream model_file(model_path);
    std::ifstream weights_file(weights_path, std::ios::binary);
    if (!model_file.is_open() || !weights_file.is_open()) {
        throw std::runtime_error("Cannot open model or weights file");
    }

    // User can add file decryption of model_file and weights_file in memory here.

    std::string model_str((std::istreambuf_iterator<char>(model_file)), std::istreambuf_iterator<char>());
    std::vector<char> weights_buffer((std::istreambuf_iterator<char>(weights_file)), std::istreambuf_iterator<char>());
    auto weights_tensor = ov::Tensor(ov::element::u8, {weights_buffer.size()}, weights_buffer.data());
    return {model_str, weights_tensor};
}

ov::genai::Tokenizer decrypt_tokenizer(const std::string& models_path) {
    std::string tok_model_path = models_path + "/openvino_tokenizer.xml";
    std::string tok_weights_path = models_path + "/openvino_tokenizer.bin";
    auto [tok_model_str, tok_weights_tensor] = decrypt_model(tok_model_path, tok_weights_path);

    std::string detok_model_path = models_path + "/openvino_detokenizer.xml";
    std::string detok_weights_path = models_path + "/openvino_detokenizer.bin";
    auto [detok_model_str, detok_weights_tensor] = decrypt_model(tok_model_path, tok_weights_path);

    return ov::genai::Tokenizer(tok_model_str, tok_weights_tensor, detok_model_str, detok_weights_tensor);
}

int main(int argc, char* argv[]) try {
    if (3 > argc)
        throw std::runtime_error(std::string{"Usage: "} + argv[0] + " <MODEL_DIR> \"<PROMPT>\"");

    std::string device = "CPU";  // GPU, NPU can be used as well
    std::string models_path = argv[1];
    std::string prompt = argv[2];

    auto [model_str, model_weights] = decrypt_model(models_path + "/openvino_model.xml", models_path + "/openvino_model.bin");
    ov::genai::Tokenizer tokenizer = decrypt_tokenizer(models_path);
    
    ov::genai::LLMPipeline pipe(model_str, model_weights, tokenizer, device);

    std::string result = pipe.generate(prompt, ov::genai::max_new_tokens(100));
    std::cout << result << std::endl;
} catch (const std::exception& error) {
    try {
        std::cerr << error.what() << '\n';
    } catch (const std::ios_base::failure&) {}
    return EXIT_FAILURE;
} catch (...) {
    try {
        std::cerr << "Non-exception object thrown\n";
    } catch (const std::ios_base::failure&) {}
    return EXIT_FAILURE;
}
