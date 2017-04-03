#include "components/sync/util/opera_tag_node_mapping.h"

namespace opera_sync {

TagNodeMappingVector tag_node_mapping() {
  TagNodeMappingVector ret;
  ret.push_back({ "userRoot", "user_root_bookmarks" });
  ret.push_back({ "shared", "shared_bookmarks" });
  ret.push_back({ "unsorted", "unsorted_bookmarks" });
  ret.push_back({ "speedDial", "speeddial_bookmarks" });
  ret.push_back({ "trash", "trash_bookmarks" });
  return ret;
}

}  // namespace opera_sync
