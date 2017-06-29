<?php
/**
 * @file
 * Defines classes for processing Gizmos.
 */

/**
 * The parent of all gizmos
 */

abstract class Gizmo {

  /** @var int db id */
  public $id;
  /** @var int machine_name */
  public $machine_name;
  /**
   * @var string Human facing name */
  public $human_name = 'No name set!';
  /**
   * @var array Default config */
  public $default_config = [];
  /** @var array holds all configuration data for this instance. */
  public $config;

  /** @var bool */
  public $expose_token;
  /** @var bool */
  public $expose_block;
  /** @var array of Gizmo objects keyed by machine name */
  public static $singleton_cache = [];
  /** Holds cache of which gizmos expose blocks. */
  public static $block_info;
  public static function createFromClass($classname) {
    $classname = static::validateClass($classname);
    $gizmo = new $classname();
    return $gizmo;
  }
  public static function createFromMachineName($machine_name) {

    if (!isset(static::$singleton_cache[$machine_name])) {
      $row = db_select('gizmo')->fields('gizmo')->condition('name', $machine_name)->execute()->fetchAssoc();
      if (!$row) {
        throw new GizmoNotFoundException("Gizmo $machine_name not found.");
      }
      $classname = static::validateClass($row['classname']);
      static::$singleton_cache[$machine_name] = new $classname($row);
    }

    return static::$singleton_cache[$machine_name];
  }
  /**
   * Create instance by loading config from database.
   */
  public static function createFromId($id) {
    $id = (int) $id;
    if (!$id) {
      throw new \InvalidArgumentException("invalid gizmo id.");
    }
    // In cache?
    foreach (static::$singleton_cache as $gizmo) {
      if ($gizmo->id == $id) {
        return $gizmo;
      }
    }
    // Not in cache yet.
    $row = db_select('gizmo')->fields('gizmo')->condition('id', $id)->execute()->fetchAssoc();
    $classname = static::validateClass($row['classname']);
    $gizmo = new $classname($row);
    // Cache it.
    static::$singleton_cache[$gizmo->machine_name] = $gizmo;
    return $gizmo;
  }
  /**
   * Create instance from table data.
   */
  public static function createFromRowData($row) {
    if (empty($row['name'])) {
      throw new \InvalidArgumentException("invalid gizmo name in row.");
    }
    if (empty(static::$singleton_cache[$row['name']])) {
      // Not in cache yet.
      $classname = static::validateClass($row['classname']);
      $gizmo = new $classname($row);
      // Cache it.
      static::$singleton_cache[$gizmo->machine_name] = $gizmo;
    }
    return static::$singleton_cache[$row['name']];
  }
  /**
   * Delete a gizmo, identified by ID.
   *
   * @param int $id
   */
  public static function deleteGizmo($id) {
    $id = (int) $id;
    if (!$id) {
      throw new InvalidArgumentException("Invalid gizmo id to delete");
    }
    db_delete('gizmo')
      ->condition('id', $id)
      ->execute();
  }
  /**
   * Ensure that the given class name is a subclass of this.
   */
  public static function validateClass($classname) {
    if (!class_exists($classname, $autoload=TRUE)) {
      throw new GizmoNotFoundException("'$classname' is not found.");
    }
    $reflection = new \ReflectionClass($classname);
    if ($reflection->isSubclassOf('Gizmo')) {
      return $classname;
    }
    throw new GizmoNotFoundException("'$classname' is not a gizmo class.");
  }
  /**
   * Implements hook_token_info.
   */
  public static function tokenInfo() {
    $info = [];

    // First define the types of token.
    // The type is typically the first part of a token, e.g. foo in [foo:bar]
    $info['types'] = [
      'gizmo' => [
        'name' => 'Gizmo',
        'description' => 'specify a particular gizmo',
      ],
      'gizmo-instance' => [
        'name' => 'Particular gizmo',
        'description' => '',
        'needs-data' => 'gizmo-instance',
      ],
    ];

    // Now define the tokens.
    $info['tokens'] = [
      'gizmo' => [], // Dynamic.
      'gizmo-instance' => [], // Dynamic.
    ];

    return $info;
  }
  /**
   * Implements hook_tokens.
   */
  public static function tokens($type, $tokens, array $data = array(), array $options = array()) {

    $replacements = [];
    if ($type == 'gizmo') {

      // Deal with all tokens for a particular instance in a batch.
      $batch = [];
      foreach ($tokens as $token => $original) {
        $parts = explode(':', $token);
        if (!isset($batch[$parts[0]])) {
          $batch[$parts[0]] = 1;
        }
      }
      foreach (array_keys($batch) as $machine_name) {
        try {
          $gizmo = static::createFromMachineName($machine_name);
          if (!$gizmo->expose_token) {
            // Tokens turned off for this gizmo.
            continue;
          }
        }
        catch (GizmoNotFoundException $e) {
          watchdog('gizmo', $e->getMessage(), [], WATCHDOG_ERROR);
          continue;
        }
        $chained_tokens = token_find_with_prefix($tokens, $machine_name);
        $replacements += $gizmo->tokenReplacements($chained_tokens, $data, $options);
      }
    }
    return $replacements;
  }
  /**
   * Provide values for tokens.
   * @param array $tokens in form key:original.
   * @return array original:value.
   */
  public function tokenReplacements($tokens, $data, $options) {

  }
  /**
   * Returns help text about tokens this gizmo makes available.
   *
   * @return string HTML.
   */
  public function tokensList() { }
  public static function blockInfo() {
    if (!isset(static::$block_info)) {
      static::$block_info = [];
      $gizmos = db_query("
        SELECT id, classname, name
        FROM gizmo
        WHERE expose_block
        ORDER BY classname, name
        ")
        ->fetchAllAssoc('id');

      foreach ($gizmos as $row) {
        try {
          $gizmo = Gizmo::createFromClass($row->classname);
        }
        catch (GizmoNotFoundException $e) {
          watchdog('gizmo', $e->getMessage(), [], WATCHDOG_ERROR);
          continue;
        }

        static::$block_info["gizmo-$row->id"] = [
          'info' => 'Gizmo: ' . $gizmo->human_name . ": $row->name",
          'cache' => DRUPAL_NO_CACHE,
          'title' => '',
        ];
        unset($gizmo);
      }
    }
    return static::$block_info;
  }
  /**
   * Get an array of instances that expose tokens.
   * @var string $expose_what expose_token or expose_block
   */
  public static function getInstances($expose_what) {


    return static::$cache_instances_tokens;
  }
  public function __construct($row=NULL) {
    if ($row !== NULL) {
      $this->loadFromStoredData($row);
    }
  }
  /**
   * Load our data from an associative array from the gizmos table.
   */
  public function loadFromStoredData($row) {
    $this->id = $row['id'];
    $this->expose_block = $row['expose_block'];
    $this->expose_token = $row['expose_token'];
    $this->machine_name = $row['name'];
    $this->setConfig(unserialize($row['phpdata']));
  }
  /**
   * Sets the config ensuring it's valid.
   *
   * This implementation simply ensures all the defaults exist, and that no
   * other keys exist, but you could do other things, especially if you need to
   * coerce some old config into a new style.
   */
  public function setConfig($config) {
    $this->config = array_intersect_key($config + $this->default_config, $this->default_config);
  }
  /**
   * Output the gizmo's block, if exposed block.
   */
  public function blockView($delta) {
    return [
      'subject' => '',
      'content' => '',
    ];
  }
  /**
   * Form callback.
   *
   */
  public function adminFormShared($form, &$form_state) {

    drupal_set_title(($this->id ? 'Edit ' : 'Create ') . $this->human_name . ' gizmo');

    $form['machine_name'] = [
      '#type' => 'textfield',
      '#size' => 60,
      '#title' => 'Name - must be all one word and unique.',
      '#required' => TRUE,
      '#default_value' => $this->machine_name,
      '#disabled' => $this->id > 0,
      '#description' => $this->id > 0 ? "This cannot be changed now." : "",
    ];

    $form['expose_token'] = [
      '#type' => 'checkbox',
      '#title' => 'Expose tokens',
      '#default_value' => $this->expose_token,
    ];

    $form['expose_block'] = [
      '#type' => 'checkbox',
      '#title' => 'Expose as block',
      '#default_value' => $this->expose_block,
    ];

    $form = $this->adminForm($form, $form_state);

    $form['save'] = [
      '#type' => 'submit',
      '#value' => $this->id ? 'Update Gizmo' : 'Create Gizmo',
      '#weight' => 1000,
    ];
    return $form;
  }
  /**
   * Form callback for extending classes.
   */
  public function adminForm($form, &$form_state) {

  }
  /**
   * Form validation.
   */
  public function adminFormValidateShared($form, &$form_state) {
    if (!preg_match('/^[a-zA-Z0-9-_]+$/', $form_state['values']['machine_name'])) {
      form_set_error('machine_name', 'The name must something_like_this or SomethingLikeThis or something-like-this etc.');
    }

    $this->adminFormValidate($form, $form_state);
  }
  /**
   * Form validation for extending classes.
   */
  public function adminFormValidate($form, &$form_state) {
  }
  /**
   * Form submit.
   */
  public function adminFormSubmit($form, &$form_state) {
    // update or create?

    $this->adminFormSetConfigFromFormValues($form_state['values']);

    $params = [
      'phpdata' => serialize($this->config),
      'classname' => get_class($this),
      'name' => $form_state['values']['machine_name'],
      'expose_block' => $form_state['values']['expose_block'],
      'expose_token' => (int) $form_state['values']['expose_token'],
    ];
    if ($this->id) {
      db_update('gizmo')
        ->fields($params)
        ->condition('id', $this->id)
        ->execute();
    }
    else {
      $success = db_insert('gizmo')
        ->fields($params)
        ->execute();
      if (!$success) {
        throw new Exception("Failed to update database");
      }
      $this->id = $success;
    }
  }
  /**
   * Convert the form values to config.
   *
   * The default implementation simply calls setConfig. But you might need to
   * do some reworking of the values.
   */
  protected function adminFormSetConfigFromFormValues($values) {
    $this->setConfig($values);
  }
}

/**
 * Exception.
 */
class GizmoNotFoundException extends Exception {}

