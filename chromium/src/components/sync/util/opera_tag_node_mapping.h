#ifndef SYNC_UTIL_OPERA_TAG_NODE_MAPPING_H_
#define SYNC_UTIL_OPERA_TAG_NODE_MAPPING_H_

#include <vector>

namespace opera_sync {
struct TagNodeMapping {
  const char* node_name;  // Name of the custom bookmark node in BookmarkModel.
  const char* tag_name;  // Name of the permanent sync item on the server.
};

typedef std::vector<TagNodeMapping> TagNodeMappingVector;

TagNodeMappingVector tag_node_mapping();
}  // namespace opera_sync

#endif  // SYNC_UTIL_OPERA_TAG_NODE_MAPPING_H_

