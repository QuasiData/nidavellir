/**
 * @brief A structure containing type information and operations for a
 * component.
 *
 * I have changed the workflow on GH and even more so now without local build
 * This structure holds metadata and function pointers for managing the
 * lifecycle of a component type. It includes functions for construction,
 * destruction, copy construction, copy assignment, move construction, move
 * assignment, and combined move and destruct operations.
 */
struct CompTypeInfo {
  /**
   * @brief The unique identifier for the component type.
   */
  int id;

  /**
   * @brief Function pointer for the default constructor.
   *
   * @param dst The destination where the components are constructed.
   * @param count The number of components to construct.
   */
  void (*ctor)(void *dst, int count);

  /**
   * @brief Function pointer for the destructor.
   *
   * @param src The source from where the components are destructed.
   * @param count The number of components to destruct.
   */
  void (*dtor)(void *src, int count);

  /**
   * @brief Function pointer for the copy constructor.
   *
   * @param dst The destination where the components are constructed.
   * @param src The source from where the components are copied.
   * @param count The number of components to copy construct.
   */
  void (*copy_ctor)(void *dst, void *src, int count);

  /**
   * @brief Function pointer for the copy assignment operator.
   *
   * @param dst The destination where the components are assigned.
   * @param src The source from where the components are copied.
   * @param count The number of components to copy assign.
   */
  void (*copy_assign)(void *dst, void *src, int count);

  /**
   * @brief Function pointer for the move constructor.
   *
   * @param dst The destination where the components are move constructed.
   * @param src The source from where the components are moved.
   * @param count The number of components to move construct.
   */
  void (*move_ctor)(void *dst, void *src, int count);

  /**
   * @brief Function pointer for the move assignment operator.
   *
   * @param dst The destination where the components are move assigned.
   * @param src The source from where the components are moved.
   * @param count The number of components to move assign.
   */
  void (*move_assign)(void *dst, void *src, int count);

  /**
   * @brief Function pointer for the combined move constructor and destructor.
   *
   * @param dst The destination where the components are move constructed.
   * @param src The source from where the components are moved and destructed.
   * @param count The number of components to move construct and destruct.
   */
  void (*move_ctor_dtor)(void *dst, void *src, int count);

  /**
   * @brief Function pointer for the combined move assignment and destructor.
   *
   * @param dst The destination where the components are move assigned.
   * @param src The source from where the components are moved and destructed.
   * @param count The number of components to move assign and destruct.
   */
  void (*move_assign_dtor)(void *dst, void *src, int count);

  /**
   * @brief The size of the component type in bytes.
   */
  int size;
};