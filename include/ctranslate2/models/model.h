#pragma once

#include <unordered_map>
#include <memory>

#include "ctranslate2/vocabulary.h"
#include "ctranslate2/vocabulary_map.h"
#include "ctranslate2/layers/encoder.h"
#include "ctranslate2/layers/decoder.h"

namespace ctranslate2 {
  namespace models {

    static const size_t current_binary_version = 4;

    // Checks whether the provided path could contain a CTranslate2 model.
    bool contains_model(const std::string& path);

    // Base class for models.
    class Model {
    public:
      static std::shared_ptr<const Model> load(const std::string& path,
                                               const std::string& device = "cpu",
                                               int device_index = 0,
                                               const std::string& compute_type = "default");
      static std::shared_ptr<const Model> load(const std::string& path,
                                               Device device = Device::CPU,
                                               int device_index = 0,
                                               ComputeType compute_type = ComputeType::DEFAULT);

      virtual ~Model() = default;
      virtual size_t current_spec_revision() const;

      Device device() const;
      int device_index() const;
      ComputeType compute_type() const;
      ScopedDeviceSetter get_scoped_device_setter() const;

      const Vocabulary& get_source_vocabulary() const;
      const Vocabulary& get_target_vocabulary() const;
      const VocabularyMap& get_vocabulary_map() const;

      const StorageView* get_variable_if_exists(const std::string& name) const;
      const StorageView& get_variable(const std::string& name) const;
      const std::unordered_map<std::string, StorageView>& get_variables() const;

      // Attributes are saved as scalar variables.
      template <typename T>
      T get_attribute_with_default(const std::string& name, T default_value) const {
        const StorageView* attribute = get_variable_if_exists(name);
        if (!attribute)
          return default_value;
        return attribute->as_scalar<T>();
      }

      // A flag is a boolean attribute.
      bool get_flag_with_default(const std::string& name, bool default_value) const;

      // Makes new graph to execute this model. Graphs returned by these function
      // should support being executed in parallel without duplicating the model
      // data (i.e. the weights).
      virtual std::unique_ptr<layers::Encoder> make_encoder() const = 0;
      virtual std::unique_ptr<layers::Decoder> make_decoder() const = 0;

    protected:
      Model(const std::string& path, size_t spec_revision);

      // Models can override these methods to execute some transformations if needed
      // (e.g. a variable name changed in a newer spec revision).
      virtual void register_variable(const std::string& name, StorageView& variable);
      virtual void register_variable_alias(const std::string& alias,
                                           const std::string& variable_name);
      virtual void finalize();
      StorageView* get_scale(const std::string& scale_name, DataType dataType);

      void set_device(Device type, int index = 0);
      void set_compute_type(ComputeType type);

      Device _device;
      int _device_index;
      std::unique_ptr<const Vocabulary> _source_vocabulary;
      std::unique_ptr<const Vocabulary> _target_vocabulary;
      std::unique_ptr<const Vocabulary> _shared_vocabulary;
      std::unique_ptr<const VocabularyMap> _vocabulary_map;
      std::unordered_map<std::string, StorageView> _variable_index;
      std::unordered_map<std::string, std::string> _variable_alias;
      size_t _spec_revision;
      ComputeType _compute_type = ComputeType::DEFAULT;

      void convert_data_if_need(bool support_int8,
                                bool support_int16,
                                const std::string& name,
                                StorageView& variable,
                                std::vector<std::pair<std::string, StorageView>>& variables_to_add,
                                std::vector<std::string>& variables_to_remove);
    };

  }
}
