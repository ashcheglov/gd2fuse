#include "DriveApiPatch.h"
#include <google/drive_api/drive_service.h>


namespace google_drive_api
{
	using namespace googleapis;

  /**
   * Get a reference to the value of the '<code>items</code>' attribute.
   */
  const client::JsonCppArray<ChildReference> ChildList::get_items() const
  {
	  return client::JsonCppArray<ChildReference>(Storage().get("items",Json::Value::null));
  }

  /**
   * Gets a reference to a mutable value of the '<code>items</code>' property.
   *
   * The actual list of children.
   *
   * @return The result can be modified to change the attribute value.
   */
  client::JsonCppArray<ChildReference> ChildList::mutable_items()
  {
	  return client::JsonCppArray<ChildReference>(MutableStorage()->get("items",Json::Value::null));
  }

}
